#include "bookmgmt/Acquisition.h"

#include <cstdint>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>
#include "bookmgmt/Exceptions.h"
#include <algorithm>
namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget)
    : catalog_(catalog), budget_(budget) {}
void AcquisitionManager::addVendorOffer(
    const std::string& resourceId,
    const std::string& vendorName,
    Money unitPrice
) {
    if (!catalog_.find(resourceId)) {
        throw NotFoundError(resourceId);
    }

    if (vendorName.empty()) {
        throw std::invalid_argument("vendor name must not be empty");
    }

    if (unitPrice.isNegative()) {
        throw std::invalid_argument("vendor price must not be negative");
    }

    auto& offers = vendorOffers_[resourceId];

    for (auto& offer : offers) {
        if (offer.vendorName == vendorName) {
            offer.unitPrice = unitPrice;
            return;
        }
    }

    offers.push_back(VendorOffer{vendorName, unitPrice});
}

const std::vector<VendorOffer>&
AcquisitionManager::vendorOffersFor(
    const std::string& resourceId
) const {
    static const std::vector<VendorOffer> emptyOffers;

    const auto it = vendorOffers_.find(resourceId);

    if (it == vendorOffers_.end()) {
        return emptyOffers;
    }

    return it->second;
}

AcquisitionManager::SelectedOffer
AcquisitionManager::cheapestOffer(const Resource& resource) const {
    const auto& offers = vendorOffersFor(resource.id());

    if (offers.empty()) {
        return {"", resource.unitPrice()};
    }

    const auto cheapest = std::min_element(
        offers.begin(),
        offers.end(),
        [](const VendorOffer& a, const VendorOffer& b) {
            return a.unitPrice < b.unitPrice;
        }
    );

    return {cheapest->vendorName, cheapest->unitPrice};
}
void AcquisitionManager::registerDepartment(
    const std::string& name, Budget& departmentBudget) {
    if (name.empty())
        throw std::invalid_argument("department name must not be empty");

    if (&departmentBudget == &budget_)
        throw std::invalid_argument("department must have its own budget");

    if (departments_.count(name))
        throw std::invalid_argument("department already registered: " + name);

    for (const auto& entry : departments_) {
        if (entry.second == &departmentBudget)
            throw std::invalid_argument("budget already assigned to department: " + entry.first);
    }

    departments_[name] = &departmentBudget;
}

Budget* AcquisitionManager::budgetFor(const std::string& department) {
    if (department.empty())
        return &budget_;

    auto it = departments_.find(department);
    return it == departments_.end() ? nullptr : it->second;
}

const Budget* AcquisitionManager::budgetFor(const std::string& department) const {
    if (department.empty())
        return &budget_;

    auto it = departments_.find(department);
    return it == departments_.end() ? nullptr : it->second;
}

void AcquisitionManager::setTaxRates(int printPercent, int electronicPercent) {
    if (printPercent < 0 || electronicPercent < 0)
        throw std::invalid_argument("tax rates must not be negative");

    printTaxPercent_ = printPercent;
    electronicTaxPercent_ = electronicPercent;
}

Money AcquisitionManager::postTaxCost(const Resource& r, Money preTaxCost) const {
    const int rate = r.isDigital()
                         ? electronicTaxPercent_
                         : printTaxPercent_;

    if (rate == 0)
        return preTaxCost;

    const std::int64_t taxMinor =
        (preTaxCost.minorUnits() * rate) / 100;

    return Money::fromMinor(preTaxCost.minorUnits() + taxMinor);
}

Money AcquisitionManager::quote(
    const std::string& id,
    int quantity
) const {
    const Resource& resource = catalog_.get(id);
    const SelectedOffer offer = cheapestOffer(resource);

    return resource.costForAtPrice(quantity, offer.unitPrice);
}

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                     std::string* reason) const {
    return canPurchase(id, quantity, std::string{}, reason);
}

bool AcquisitionManager::canPurchase(
    const std::string& id,
    int quantity,
    const std::string& department,
    std::string* reason
) const {
    std::string why;

    const Resource* resource = catalog_.find(id);

    if (!resource) {
        why = "resource not found: " + id;
    } else if (quantity <= 0) {
        why = "quantity must be positive";
    } else {
        const Budget* selectedBudget = budgetFor(department);

        if (!selectedBudget) {
            why = "department not found: " + department;
        } else {
            const SelectedOffer offer = cheapestOffer(*resource);

            const Money preTaxCost =
                resource->costForAtPrice(quantity, offer.unitPrice);

            const Money cost = postTaxCost(*resource, preTaxCost);

            why = selectedBudget->check(
                resource->category(),
                quantity,
                cost,
                resource->title()
            );
        }
    }

    if (reason) {
        *reason = why;
    }

    return why.empty();
}

PurchaseRecord& AcquisitionManager::record(
    const Resource* r,
    const std::string& id,
    int qty,
    Money preTaxCost,
    Money cost,
    bool approved,
    std::string reason,
    const std::string& department,
    const std::string& vendor
) {
    history_.push_back(PurchaseRecord{
        nextOrderNo_++,
        id,
        r ? r->title() : std::string("(unknown)"),
        r ? r->category() : ResourceCategory::Book,
        qty,
        preTaxCost,
        cost,
        approved,
        std::move(reason),
        department,
        vendor
    });

    return history_.back();
}

const PurchaseRecord& AcquisitionManager::purchase(const std::string& id,
                                                   int quantity) {
    return purchase(id, quantity, std::string{});
}

const PurchaseRecord& AcquisitionManager::purchase(
    const std::string& id,
    int quantity,
    const std::string& department
) {
    Budget* selectedBudget = budgetFor(department);

    if (!selectedBudget) {
        throw std::invalid_argument(
            "department not found: " + department
        );
    }

    const Resource& resource = catalog_.get(id);
    const SelectedOffer offer = cheapestOffer(resource);

    const Money preTaxCost =
        resource.costForAtPrice(quantity, offer.unitPrice);

    const Money cost = postTaxCost(resource, preTaxCost);

    selectedBudget->commit(
        resource.category(),
        quantity,
        cost,
        resource.title()
    );

    catalog_.addHoldings(id, quantity);

    return record(
        &resource,
        id,
        quantity,
        preTaxCost,
        cost,
        true,
        {},
        department,
        offer.vendorName
    );
}

const PurchaseRecord& AcquisitionManager::cancelOrder(int orderNo) {
    const PurchaseRecord* target = nullptr;

    for (const auto& rec : history_) {
        if (rec.orderNo == orderNo) {
            target = &rec;
            break;
        }
    }

    if (!target) {
        throw std::invalid_argument("order not found: " +
                                    std::to_string(orderNo));
    }

    if (target->cancellation) {
        throw std::invalid_argument(
            "cannot cancel a cancellation record"
        );
    }

    if (!target->approved) {
        throw std::invalid_argument(
            "only approved orders can be cancelled"
        );
    }

    for (const auto& rec : history_) {
        if (rec.cancellation && rec.relatedOrderNo == orderNo) {
            throw std::invalid_argument(
                "order has already been cancelled"
            );
        }
    }

    const PurchaseRecord original = *target;
    Budget* selectedBudget = budgetFor(original.department);

    if (!selectedBudget)
        throw std::invalid_argument("department not found: " + original.department);

    const Resource* r = catalog_.find(original.resourceId);

    if (!r) {
        throw NotFoundError(original.resourceId);
    }

    if (catalog_.holdings(original.resourceId) < original.quantity) {
        throw std::invalid_argument(
            "holdings are insufficient to cancel order"
        );
    }

    selectedBudget->refund(
        original.category,
        original.quantity,
        original.cost,
        original.title
    );

    catalog_.addHoldings(
        original.resourceId,
        -original.quantity
    );

       PurchaseRecord& cancellationRecord = record(
        r,
        original.resourceId,
        original.quantity,
        Money::fromMinor(-original.preTaxCost.minorUnits()),
        Money::fromMinor(-original.cost.minorUnits()),
        true,
        {},
        original.department,
        original.vendor
    );

    cancellationRecord.cancellation = true;
    cancellationRecord.relatedOrderNo = orderNo;

    return cancellationRecord;
}

std::vector<PurchaseRecord> AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& reqs,
    bool allOrNothing
) {
    if (allOrNothing) {
        struct PendingRequest {
            const Resource* resource;
            PurchaseRequest request;
            Money preTaxCost;
            Money cost;
            std::string vendor;
            std::string reason;
        };

        std::vector<PendingRequest> pending;
        pending.reserve(reqs.size());

        Budget simulatedDefault = budget_;
        std::map<std::string, Budget> simulatedDepartments;

        for (const auto& entry : departments_) {
            simulatedDepartments.emplace(entry.first, *entry.second);
        }

        std::size_t firstRejected = reqs.size();

        for (std::size_t i = 0; i < reqs.size(); ++i) {
            const PurchaseRequest& req = reqs[i];
            const Resource* resource = catalog_.find(req.resourceId);

            Money preTaxCost;
            Money cost;
            std::string vendor;
            std::string why;

            if (!resource) {
                why = "resource not found: " + req.resourceId;
            } else if (req.quantity <= 0) {
                why = "quantity must be positive";
            } else {
                const SelectedOffer offer = cheapestOffer(*resource);
                vendor = offer.vendorName;

                preTaxCost =
                    resource->costForAtPrice(req.quantity, offer.unitPrice);

                cost = postTaxCost(*resource, preTaxCost);

                Budget* selectedBudget = nullptr;

                if (req.department.empty()) {
                    selectedBudget = &simulatedDefault;
                } else {
                    auto it = simulatedDepartments.find(req.department);

                    if (it != simulatedDepartments.end()) {
                        selectedBudget = &it->second;
                    }
                }

                if (!selectedBudget) {
                    why = "department not found: " + req.department;
                } else {
                    why = selectedBudget->check(
                        resource->category(),
                        req.quantity,
                        cost,
                        resource->title()
                    );

                    if (why.empty()) {
                        selectedBudget->commit(
                            resource->category(),
                            req.quantity,
                            cost,
                            resource->title()
                        );
                    }
                }
            }

            if (!why.empty() && firstRejected == reqs.size()) {
                firstRejected = i;
            }

            pending.push_back(PendingRequest{
                resource,
                req,
                preTaxCost,
                cost,
                std::move(vendor),
                std::move(why)
            });
        }

        std::vector<PurchaseRecord> results;
        results.reserve(reqs.size());

        if (firstRejected != reqs.size()) {
            const std::string abortReason =
                "batch aborted because request " +
                std::to_string(firstRejected + 1) +
                " would be rejected";

            for (const auto& item : pending) {
                std::string reason = item.reason;

                if (reason.empty()) {
                    reason = abortReason;
                }

                results.push_back(record(
                    item.resource,
                    item.request.resourceId,
                    item.request.quantity,
                    item.preTaxCost,
                    item.cost,
                    false,
                    std::move(reason),
                    item.request.department,
                    item.vendor
                ));
            }

            return results;
        }

        for (const auto& req : reqs) {
            results.push_back(purchase(
                req.resourceId,
                req.quantity,
                req.department
            ));
        }

        return results;
    }

    // Existing behaviour: process each request independently.
    std::vector<PurchaseRecord> results;
    results.reserve(reqs.size());

    for (const auto& req : reqs) {
        const Resource* resource = catalog_.find(req.resourceId);

        Money preTaxCost;
        Money cost;
        std::string vendor;
        std::string why;

        if (!resource) {
            why = "resource not found: " + req.resourceId;
        } else if (req.quantity <= 0) {
            why = "quantity must be positive";
        } else {
            const SelectedOffer offer = cheapestOffer(*resource);
            vendor = offer.vendorName;

            preTaxCost =
                resource->costForAtPrice(req.quantity, offer.unitPrice);

            cost = postTaxCost(*resource, preTaxCost);

            const Budget* selectedBudget = budgetFor(req.department);

            if (!selectedBudget) {
                why = "department not found: " + req.department;
            } else {
                why = selectedBudget->check(
                    resource->category(),
                    req.quantity,
                    cost,
                    resource->title()
                );
            }
        }

        if (why.empty()) {
            results.push_back(purchase(
                req.resourceId,
                req.quantity,
                req.department
            ));
        } else {
            results.push_back(record(
                resource,
                req.resourceId,
                req.quantity,
                preTaxCost,
                cost,
                false,
                why,
                req.department,
                vendor
            ));
        }
    }

    return results;
}

Money AcquisitionManager::totalSpent() const {
    Money sum;

    for (const auto& rec : history_) {
        if (rec.approved)
            sum += rec.cost;
    }

    return sum;
}

void AcquisitionManager::printReport(std::ostream& os) const {
    os << "Order history (" << history_.size() << " orders)\n";

    for (const auto& rec : history_) {
        os << "  #" << std::setw(3) << std::left << rec.orderNo << " "
           << (rec.cancellation ? "CANCELLED" : (rec.approved ? "APPROVED" : "REJECTED")) << "  "
           << std::setw(6) << rec.resourceId
           << " x" << std::setw(3) << rec.quantity
           << " pre-tax " << rec.preTaxCost.toString()
           << " post-tax " << rec.cost.toString()
           << "  " << rec.title;
             if (!rec.vendor.empty()) {
            os << "  vendor: " << rec.vendor;
        }

        if (!rec.department.empty())
            os << "  department: " << rec.department;

        if (rec.cancellation) {
            os << "\n        cancels order: #" << rec.relatedOrderNo;
        } else if (!rec.approved) {
            os << "\n        reason: " << rec.reason;
        }

        os << "\n";
    }

    Money totalPreTax;

    for (const auto& rec : history_) {
        if (rec.approved) {
            totalPreTax += rec.preTaxCost;
        }
    }

    os << "Total pre-tax: " << totalPreTax << "\n";
    os << "Total post-tax: " << totalSpent() << "\n";
    os << "Total spent: " << totalSpent() << "\n";
}

}  // namespace bookmgmt
