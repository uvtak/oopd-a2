#include "bookmgmt/Acquisition.h"

#include <cstdint>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>
#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget)
    : catalog_(catalog), budget_(budget) {}

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

Money AcquisitionManager::quote(const std::string& id, int quantity) const {
    return catalog_.get(id).costFor(quantity);
}

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                     std::string* reason) const {
    return canPurchase(id, quantity, std::string{}, reason);
}

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                     const std::string& department,
                                     std::string* reason) const {
    std::string why;

    if (const Resource* r = catalog_.find(id)) {
        if (quantity <= 0) {
            why = "quantity must be positive";
        } else {
            const Budget* selectedBudget = budgetFor(department);

            if (!selectedBudget) {
                why = "department not found: " + department;
            } else {
                const Money preTaxCost = r->costFor(quantity);
                const Money cost = postTaxCost(*r, preTaxCost);

                why = selectedBudget->check(
                    r->category(), quantity, cost, r->title());
            }
        }
    } else {
        why = "resource not found: " + id;
    }

    if (reason)
        *reason = why;

    return why.empty();
}

PurchaseRecord& AcquisitionManager::record(const Resource* r,
                                           const std::string& id,
                                           int qty,
                                           Money preTaxCost,
                                           Money cost,
                                           bool approved,
                                           std::string reason,
                                           const std::string& department) {
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
        department
    });

    return history_.back();
}

const PurchaseRecord& AcquisitionManager::purchase(const std::string& id,
                                                   int quantity) {
    return purchase(id, quantity, std::string{});
}

const PurchaseRecord& AcquisitionManager::purchase(const std::string& id,
                                                   int quantity,
                                                   const std::string& department) {
    Budget* selectedBudget = budgetFor(department);

    if (!selectedBudget)
        throw std::invalid_argument("department not found: " + department);

    const Resource& r = catalog_.get(id);

    const Money preTaxCost = r.costFor(quantity);
    const Money cost = postTaxCost(r, preTaxCost);

    selectedBudget->commit(r.category(), quantity, cost, r.title());
    catalog_.addHoldings(id, quantity);

    return record(&r, id, quantity, preTaxCost, cost, true, {}, department);
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
        original.department
    );

    cancellationRecord.cancellation = true;
    cancellationRecord.relatedOrderNo = orderNo;

    return cancellationRecord;
}

std::vector<PurchaseRecord> AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& reqs) {
    std::vector<PurchaseRecord> results;
    results.reserve(reqs.size());

    for (const auto& req : reqs) {
        const Resource* r = catalog_.find(req.resourceId);
        const Budget* selectedBudget = budgetFor(req.department);

        Money preTaxCost;
        Money cost;
        std::string why;

        if (!r) {
            why = "resource not found: " + req.resourceId;
        } else if (req.quantity <= 0) {
            why = "quantity must be positive";
        } else {
            preTaxCost = r->costFor(req.quantity);
            cost = postTaxCost(*r, preTaxCost);

            if (!selectedBudget) {
                why = "department not found: " + req.department;
            } else {
                why = selectedBudget->check(
                    r->category(), req.quantity, cost, r->title());
            }
        }

        if (why.empty()) {
            results.push_back(
                purchase(req.resourceId, req.quantity, req.department));
        } else {
            results.push_back(
                record(r, req.resourceId, req.quantity,
                       preTaxCost, cost, false, why, req.department)
            );
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
