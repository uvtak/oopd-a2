#pragma once

#include "bookmgmt/Journal.h"

namespace bookmgmt {

class Magazine : public Journal {
public:
    Magazine(std::string id, std::string title, std::string issn,
             int issuesPerYear, std::string publisher, int year,
             Money unitPrice, int subscriptionYears = 1,
             Money postagePerIssue = Money{});

    Money postagePerIssue() const { return postagePerIssue_; }

    Money costFor(int copies) const override;
    Money costForAtPrice(int copies, Money unitPrice) const override;
protected:
    void printDetails(std::ostream& os) const override;

private:
    Money postagePerIssue_;
};

}  // namespace bookmgmt