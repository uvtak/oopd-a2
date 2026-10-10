#include "bookmgmt/ElectronicResource.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

const char* licenseName(LicenseModel m) {
    switch (m) {
        case LicenseModel::Perpetual: return "Perpetual";
        case LicenseModel::AnnualSubscription: return "Annual subscription";
    }
    return "Unknown";
}

ElectronicResource::ElectronicResource(std::string id, std::string title,
                                       std::string publisher, int year,
                                       Money pricePerSeat, std::string accessUrl,
                                       LicenseModel license, Money platformFee)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, pricePerSeat),
      accessUrl_(std::move(accessUrl)),
      license_(license),
      platformFee_(platformFee) {
    if (platformFee_.isNegative())
        throw std::invalid_argument("platform fee must not be negative");
}

Money ElectronicResource::costFor(int seats) const {
    return costForAtPrice(seats, unitPrice());
}

Money ElectronicResource::costForAtPrice(
    int seats, Money price
) const {
    requirePositive(seats);

    if (price.isNegative()) {
        throw std::invalid_argument("price must not be negative");
    }

    if (seats <= 50) {
        return platformFee_ + price * seats;
    }

    const std::int64_t first50 = price.minorUnits() * 50;
    const std::int64_t extraSeats = seats - 50;
    const std::int64_t extraCost =
        (price.minorUnits() * extraSeats) / 2;

    return platformFee_ + Money::fromMinor(first50 + extraCost);
}

void ElectronicResource::printDetails(std::ostream& os) const {
    os << "  access url: " << accessUrl_ << "\n"
       << "  license: " << licenseName(license_) << "\n"
       << "  platform fee: " << platformFee_ << "\n";
}

}  // namespace bookmgmt
