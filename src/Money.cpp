
#include "bookmgmt/Money.h"

#include <cctype>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

namespace {

std::string normalizeCurrencyCode(std::string code) {
    if (code.size() != 3) {
        throw std::invalid_argument(
            "currency code must contain three letters"
        );
    }

    for (char& character : code) {
        const unsigned char ch =
            static_cast<unsigned char>(character);

        if (!std::isalpha(ch)) {
            throw std::invalid_argument(
                "currency code must contain three letters"
            );
        }

        character = static_cast<char>(std::toupper(ch));
    }

    return code;
}

}  // namespace

Money::Money(std::int64_t minor, std::string currency)
    : minor_(minor),
      currency_(normalizeCurrencyCode(std::move(currency))) {}

Money Money::fromMinor(std::int64_t minor, std::string currency) {
    return Money(minor, std::move(currency));
}

Money Money::of(
    std::int64_t major,
    int minor,
    std::string currency
) {
    if (minor < 0 || minor > 99) {
        throw std::invalid_argument(
            "minor part must be in 0..99"
        );
    }

    const std::int64_t sign = major < 0 ? -1 : 1;

    return Money(
        major * 100 + sign * minor,
        std::move(currency)
    );
}

void Money::requireSameCurrency(const Money& other) const {
    if (currency_ != other.currency_) {
        throw std::invalid_argument(
            "cannot operate on different currencies: " +
            currency_ + " and " + other.currency_
        );
    }
}

std::string Money::toString() const {
    const std::int64_t absMinor =
        minor_ < 0 ? -minor_ : minor_;

    std::string result = std::to_string(absMinor / 100);

    const auto fraction = absMinor % 100;

    result += '.';

    if (fraction < 10) {
        result += '0';
    }

    result += std::to_string(fraction);

    return minor_ < 0 ? "-" + result : result;
}

std::ostream& operator<<(std::ostream& os, Money money) {
    return os << money.toString();
}

}  // namespace bookmgmt
