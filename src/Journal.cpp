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
    return costForAtPrice(copies, unitPrice());
}

Money Journal::costForAtPrice(int copies, Money price) const {
    requirePositive(copies);

    if (price.isNegative()) {
        throw std::invalid_argument("price must not be negative");
    }

    Money total = price * copies * subscriptionYears_;

    if (copies >= 10) {
        return Money::fromMinor((total.minorUnits() * 9) / 10, total.currencyCode());
    }

    return total;
}

void Journal::printDetails(std::ostream& os) const {
    os << "  issn: " << issn_ << "\n"
       << "  issues per year: " << issuesPerYear_ << "\n"
       << "  subscription years: " << subscriptionYears_ << "\n";
}

}  // namespace bookmgmt