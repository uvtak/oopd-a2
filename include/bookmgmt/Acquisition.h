#pragma once

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"

namespace bookmgmt {

struct PurchaseRequest {
    std::string resourceId;
    int quantity;
    std::string department{};
};
struct VendorOffer {
    std::string vendorName;
    Money unitPrice;
};
struct PurchaseRecord {
    int orderNo;
    std::string resourceId;
    std::string title;
    ResourceCategory category;
    int quantity;

    Money preTaxCost;
    Money cost;

    bool approved;
    std::string reason;
    std::string department;
    std::string vendor;
    bool cancellation = false;
    int relatedOrderNo = 0;
};

class AcquisitionManager {
public:
    AcquisitionManager(Catalog& catalog, Budget& budget);

    void registerDepartment(const std::string& name,
                            Budget& departmentBudget);

    void setTaxRates(int printPercent, int electronicPercent);

    int printTaxPercent() const { return printTaxPercent_; }
    int electronicTaxPercent() const { return electronicTaxPercent_; }

    Money quote(const std::string& id, int quantity) const;

    bool canPurchase(const std::string& id, int quantity,
                     std::string* reason = nullptr) const;

    bool canPurchase(const std::string& id, int quantity,
                     const std::string& department,
                     std::string* reason = nullptr) const;

    const PurchaseRecord& cancelOrder(int orderNo);

    const PurchaseRecord& purchase(const std::string& id, int quantity);

    const PurchaseRecord& purchase(const std::string& id, int quantity,
                                   const std::string& department);

// Official PDF Q11: All-or-nothing batch processing.
// If any request is rejected, no purchase is made.
std::vector<PurchaseRecord> processBatch(
    const std::vector<PurchaseRequest>& reqs,
    bool allOrNothing = false
);

    const std::vector<PurchaseRecord>& history() const { return history_; }

    Money totalSpent() const;

    void printReport(std::ostream& os) const;
    void addVendorOffer(
    const std::string& resourceId,
    const std::string& vendorName,
    Money unitPrice
);

    const std::vector<VendorOffer>& vendorOffersFor(
    const std::string& resourceId
) const;

private:
    struct SelectedOffer {
    std::string vendorName;
    Money unitPrice;
};

    SelectedOffer cheapestOffer(const Resource& resource) const;
    PurchaseRecord& record(const Resource* r, const std::string& id,
                           int qty, Money preTaxCost, Money cost,
                           bool approved, std::string reason,
                           const std::string& department, const std::string& vendor);

    Budget* budgetFor(const std::string& department);
    const Budget* budgetFor(const std::string& department) const;

    Money postTaxCost(const Resource& r, Money preTaxCost) const;

    int printTaxPercent_ = 0;
    int electronicTaxPercent_ = 0;

    Catalog& catalog_;
    Budget& budget_;
    std::map<std::string, Budget*> departments_;
    std::vector<PurchaseRecord> history_;
    std::map<std::string, std::vector<VendorOffer>> vendorOffers_;
    int nextOrderNo_ = 1;
};

}  // namespace bookmgmt
