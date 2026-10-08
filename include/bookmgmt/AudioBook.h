#pragma once

#include <string>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

class AudioBook : public ElectronicResource {
public:
    AudioBook(std::string id, std::string title,
              std::string narrator, int durationMinutes,
              std::string publisher, int year,
              Money unitPrice, std::string accessUrl,
              LicenseModel license = LicenseModel::AnnualSubscription,
              Money platformFee = Money{});

    const std::string& narrator() const { return narrator_; }
    int durationMinutes() const { return durationMinutes_; }

    ResourceCategory category() const override {
        return ResourceCategory::AudioBook;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string narrator_;
    int durationMinutes_;
};

/*
 * Design choice:
 * AudioBook inherits from ElectronicResource because an audiobook is
 * a digital resource and therefore can use the existing digital-resource
 * behaviour such as access URL, licensing, platform fee and seat-based pricing.
 */

}  // namespace bookmgmt