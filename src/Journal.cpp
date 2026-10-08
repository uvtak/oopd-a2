#include "bookmgmt/Journal.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

Journal::Journal(std::string id, std::string title, std::string issn,
                 int issuesPerYear, std::string publisher, int year,
                 Money unitPrice, int subscriptionYears)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, unitPrice),
      issn_(std::move(issn)),
      issuesPerYear_(issuesPerYear),
      subscriptionYears_(subscriptionYears) {
    if (subscriptionYears_ < 1)
        throw std::invalid_argument("subscription years must be >= 1");
}

Money Journal::costFor(int copies) const {
    requirePositive(copies);

    Money total = unitPrice() * copies * subscriptionYears_;

    if (copies >= 10) {
        // 10% bulk discount for print items.
        return Money::fromMinor((total.minorUnits() * 9) / 10);
    }

    return total;
}

void Journal::printDetails(std::ostream& os) const {
    os << "  issn: " << issn_ << "\n"
       << "  issues per year: " << issuesPerYear_ << "\n"
       << "  subscription years: " << subscriptionYears_ << "\n";
}

}  // namespace bookmgmt