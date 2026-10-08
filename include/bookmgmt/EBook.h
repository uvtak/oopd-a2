#pragma once

#include <string>
#include <vector>
#include "bookmgmt/Book.h"
#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

class EBook : public ElectronicResource {
public:
    EBook(std::string id, std::string title,
          std::vector<std::string> authors,
          std::string isbn, std::string publisher,
          int year, Money pricePerSeat, std::string accessUrl,
          LicenseModel license = LicenseModel::AnnualSubscription,
          Money platformFee = Money{},
          std::string format = "PDF",
          bool drmProtected = false);

    const std::vector<std::string>& authors() const { return authors_; }
    const std::string& isbn() const { return isbn_; }
    const std::string& format() const { return format_; }
    bool drmProtected() const { return drmProtected_; }

    ResourceCategory category() const override {
        return ResourceCategory::EBook;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::vector<std::string> authors_;
    std::string isbn_;
    std::string format_;
    bool drmProtected_;
};

}  // namespace bookmgmt