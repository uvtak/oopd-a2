#include "bookmgmt/Magazine.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

Magazine::Magazine(std::string id, std::string title, std::string issn,
                   int issuesPerYear, std::string publisher, int year,
                   Money unitPrice, int subscriptionYears,
                   Money postagePerIssue)
    : Journal(std::move(id), std::move(title), std::move(issn),
              issuesPerYear, std::move(publisher), year, unitPrice,
              subscriptionYears),
      postagePerIssue_(postagePerIssue) {
    if (postagePerIssue_.isNegative())
        throw std::invalid_argument("postage per issue must not be negative");
}

Money Magazine::costFor(int copies) const {
    requirePositive(copies);

    const Money subscriptionCost =
        unitPrice() * copies * subscriptionYears();

    const Money postage =
        postagePerIssue_ * issuesPerYear() * copies * subscriptionYears();

    const Money total = subscriptionCost + postage;

    if (copies >= 10) {
        return Money::fromMinor((total.minorUnits() * 9) / 10);
    }

    return total;
}

void Magazine::printDetails(std::ostream& os) const {
    Journal::printDetails(os);
    os << "  postage per issue: " << postagePerIssue_ << "\n";
}

}  // namespace bookmgmt
