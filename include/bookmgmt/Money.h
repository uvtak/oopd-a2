
#pragma once

// Money stores an exact amount in minor units (paise/cents)
// along with its currency code.

#include <cstdint>
#include <iosfwd>
#include <string>

namespace bookmgmt {

class Money {
public:
    Money() = default;

    // Construct from minor units. Default currency is INR.
    static Money fromMinor(
        std::int64_t minor,
        std::string currency = "INR"
    );

    // Construct from major and minor parts.
    static Money of(
        std::int64_t major,
        int minor = 0,
        std::string currency = "INR"
    );

    std::int64_t minorUnits() const { return minor_; }
    double toDouble() const {
        return static_cast<double>(minor_) / 100.0;
    }

    const std::string& currencyCode() const {
        return currency_;
    }

    std::string toString() const;

    bool isZero() const { return minor_ == 0; }
    bool isNegative() const { return minor_ < 0; }

    Money& operator+=(Money other) {
        requireSameCurrency(other);
        minor_ += other.minor_;
        return *this;
    }

    Money& operator-=(Money other) {
        requireSameCurrency(other);
        minor_ -= other.minor_;
        return *this;
    }

    Money& operator*=(std::int64_t factor) {
        minor_ *= factor;
        return *this;
    }

    friend Money operator+(Money a, Money b) {
        return a += b;
    }

    friend Money operator-(Money a, Money b) {
        return a -= b;
    }

    friend Money operator*(Money a, std::int64_t factor) {
        return a *= factor;
    }

    friend Money operator*(std::int64_t factor, Money a) {
        return a *= factor;
    }

    friend bool operator==(Money a, Money b) {
        a.requireSameCurrency(b);
        return a.minor_ == b.minor_;
    }

    friend bool operator!=(Money a, Money b) {
        a.requireSameCurrency(b);
        return a.minor_ != b.minor_;
    }

    friend bool operator<(Money a, Money b) {
        a.requireSameCurrency(b);
        return a.minor_ < b.minor_;
    }

    friend bool operator<=(Money a, Money b) {
        a.requireSameCurrency(b);
        return a.minor_ <= b.minor_;
    }

    friend bool operator>(Money a, Money b) {
        a.requireSameCurrency(b);
        return a.minor_ > b.minor_;
    }

    friend bool operator>=(Money a, Money b) {
        a.requireSameCurrency(b);
        return a.minor_ >= b.minor_;
    }

private:
    explicit Money(std::int64_t minor, std::string currency);

    void requireSameCurrency(const Money& other) const;

    std::int64_t minor_ = 0;
    std::string currency_ = "INR";
};

std::ostream& operator<<(std::ostream& os, Money money);

}  // namespace bookmgmt
