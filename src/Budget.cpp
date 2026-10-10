
#include "bookmgmt/Budget.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

namespace {

// Every category, in the order Budget::print() lists them.
const ResourceCategory kAllCategories[] = {
    ResourceCategory::Book,
    ResourceCategory::ElectronicResource,
    ResourceCategory::Journal,
    ResourceCategory::EBook,
    ResourceCategory::AudioBook,
    ResourceCategory::Thesis
};

bool passesEightyPercent(long double used, long double limit) {
    if (limit == 0.0L) {
        return used > 0.0L;
    }

    return used / limit > 0.8L;
}

}

Budget::Budget(Money total) : total_(total), spent_(Money::fromMinor(0, total.currencyCode())) {
    if (total_.isNegative()) {
        throw std::invalid_argument("budget must not be negative");
    }
}

void Budget::setQuota(ResourceCategory c, Quota q) {
    if (q.maxUnits < 0 ||
        q.maxSpend.isNegative() ||
        (q.maxTitles && *q.maxTitles < 0)) {
        throw std::invalid_argument("quota limits must not be negative");
    }

    quotas_[c] = q;
}

void Budget::removeQuota(ResourceCategory c) {
    quotas_.erase(c);
}

std::optional<Quota> Budget::quotaFor(ResourceCategory c) const {
    auto it = quotas_.find(c);

    if (it == quotas_.end()) {
        return std::nullopt;
    }

    return it->second;
}

Usage Budget::usageFor(ResourceCategory c) const {
    auto it = usage_.find(c);

    if (it != usage_.end()) {
        return it->second;
    }

    return Usage{
        0,
        Money::fromMinor(0, total_.currencyCode()),
        0
    };
}

std::optional<int> Budget::unitsRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);

    if (!q) {
        return std::nullopt;
    }

    return q->maxUnits - usageFor(c).units;
}

std::optional<Money> Budget::spendRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);

    if (!q) {
        return std::nullopt;
    }

    return q->maxSpend - usageFor(c).spent;
}

Budget::Failure Budget::evaluate(
    ResourceCategory c,
    int units,
    Money cost,
    const std::string& title,
    std::string& why
) const {
    if (units <= 0) {
        why = "quantity must be positive";
        return Failure::BadInput;
    }

    if (cost.isNegative()) {
        why = "cost must not be negative";
        return Failure::BadInput;
    }

    // Check the unit quota.
    if (auto left = unitsRemaining(c); left && units > *left) {
        why = std::string(categoryName(c)) +
              " unit quota exceeded: requested " +
              std::to_string(units) + ", " +
              std::to_string(*left) + " remaining";

        return Failure::Quota;
    }

    // Check the maximum number of different titles.
    const auto q = quotaFor(c);

    if (q && q->maxTitles) {
        if (title.empty()) {
            why = "title must be provided when a title quota is set";
            return Failure::BadInput;
        }

        const auto it = titles_.find(c);

        const bool alreadyBought =
            it != titles_.end() &&
            it->second.count(title) > 0;

        if (!alreadyBought &&
            usageFor(c).titles >= *q->maxTitles) {
            why = std::string(categoryName(c)) +
                  " title quota exceeded: limit " +
                  std::to_string(*q->maxTitles);

            return Failure::Quota;
        }
    }

    // Check the category spending quota.
    if (auto left = spendRemaining(c); left && cost > *left) {
        why = std::string(categoryName(c)) +
              " spend quota exceeded: cost " +
              cost.toString() + ", " +
              left->toString() + " remaining";

        return Failure::Quota;
    }

    // Check the overall budget.
    if (cost > remaining()) {
        why = "overall budget exceeded: cost " +
              cost.toString() + ", " +
              remaining().toString() + " remaining";

        return Failure::Overall;
    }

    why.clear();
    return Failure::None;
}

std::string Budget::check(
    ResourceCategory c,
    int units,
    Money cost,
    const std::string& title
) const {
    std::string why;

    evaluate(c, units, cost, title, why);

    return why;
}

void Budget::commit(
    ResourceCategory c,
    int units,
    Money cost,
    const std::string& title
) {
    std::string why;

    switch (evaluate(c, units, cost, title, why)) {
        case Failure::None:
            break;

        case Failure::BadInput:
            throw std::invalid_argument(why);

        case Failure::Quota:
            throw QuotaExceededError(why);

        case Failure::Overall:
            throw BudgetExceededError(why);
    }

    auto usageIt = usage_.find(c);

if (usageIt == usage_.end()) {
    usageIt = usage_.emplace(
        c,
        Usage{
            0,
            Money::fromMinor(0, total_.currencyCode()),
            0
        }
    ).first;
}

Usage& u = usageIt->second;
    if (!title.empty()) {
        int& activeUnits = titleUnits_[c][title];

        if (activeUnits == 0) {
            titles_[c].insert(title);
            ++u.titles;
        }

        activeUnits += units;
    }

    u.units += units;
    u.spent += cost;
    spent_ += cost;
}

void Budget::refund(
    ResourceCategory c,
    int units,
    Money cost,
    const std::string& title
) {
    if (units <= 0) {
        throw std::invalid_argument("refund quantity must be positive");
    }

    if (cost.isNegative()) {
        throw std::invalid_argument("refund cost must not be negative");
    }

    const Usage current = usageFor(c);

    if (units > current.units) {
        throw std::invalid_argument("refund units exceed recorded units");
    }

    if (cost > current.spent || cost > spent_) {
        throw std::invalid_argument("refund cost exceeds recorded spending");
    }

    auto categoryIt = titleUnits_.end();
    auto titleIt = std::map<std::string, int>::iterator{};

    if (!title.empty()) {
        categoryIt = titleUnits_.find(c);

        if (categoryIt == titleUnits_.end()) {
            throw std::invalid_argument("title has no active purchases");
        }

        titleIt = categoryIt->second.find(title);

        if (titleIt == categoryIt->second.end() ||
            units > titleIt->second) {
            throw std::invalid_argument(
                "refund units exceed active units for title"
            );
        }
    } else {
        const auto trackedTitles = titleUnits_.find(c);

        if (trackedTitles != titleUnits_.end() &&
            !trackedTitles->second.empty()) {
            throw std::invalid_argument(
                "title must be provided to refund a title-tracked purchase"
            );
        }
    }

    Usage& u = usage_[c];

    u.units -= units;
    u.spent -= cost;
    spent_ -= cost;

    if (!title.empty()) {
        titleIt->second -= units;

        if (titleIt->second == 0) {
            categoryIt->second.erase(titleIt);

            auto titlesIt = titles_.find(c);

            if (titlesIt != titles_.end()) {
                if (titlesIt->second.erase(title) > 0) {
                    --u.titles;
                }

                if (titlesIt->second.empty()) {
                    titles_.erase(titlesIt);
                }
            }

            if (categoryIt->second.empty()) {
                titleUnits_.erase(categoryIt);
            }
        }
    }
}

Budget Budget::rolloverToNextYear(
    Money nextYearBase,
    int carryOverPercent
) const {
    if (nextYearBase.isNegative()) {
        throw std::invalid_argument(
            "next year's base budget must not be negative"
        );
    }

    if (carryOverPercent < 0 || carryOverPercent > 100) {
        throw std::invalid_argument(
            "rollover percentage must be between 0 and 100"
        );
    }

    const std::int64_t unspentMinor = remaining().minorUnits();

    // Calculate the percentage without multiplying the full amount first.
    const std::int64_t carryOverMinor =
        (unspentMinor / 100) * carryOverPercent +
        ((unspentMinor % 100) * carryOverPercent) / 100;

    return Budget(
        nextYearBase + Money::fromMinor(carryOverMinor)
    );
}

void Budget::print(std::ostream& os) const {
    os << "Budget: total " << total_
       << ", spent " << spent_
       << ", remaining " << remaining() << "\n";

    os << std::left
       << std::setw(22) << "  Category"
       << std::setw(18) << "Units used/max"
       << std::setw(18) << "Titles used/max"
       << "Spend used/max\n";

    for (ResourceCategory c : kAllCategories) {
        const Usage u = usageFor(c);
        const auto q = quotaFor(c);

        const std::string units =
            std::to_string(u.units) + "/" +
            (q ? std::to_string(q->maxUnits) : "-");

        const std::string titles =
            std::to_string(u.titles) + "/" +
            (q && q->maxTitles
                 ? std::to_string(*q->maxTitles)
                 : "-");

        const std::string spend =
            u.spent.toString() + "/" +
            (q ? q->maxSpend.toString() : "-");

        os << "  "
           << std::setw(20) << categoryName(c)
           << std::setw(18) << units
           << std::setw(18) << titles
           << spend << "\n";
    }
    
    bool anyWarnings = false;

    const auto printWarning = [&os, &anyWarnings](
        const std::string& message
    ) {
        if (!anyWarnings) {
            os << "\nWarnings:\n";
            anyWarnings = true;
        }

        os << "  WARNING: " << message << "\n";
    };

    for (ResourceCategory c : kAllCategories) {
        const auto q = quotaFor(c);

        if (!q) {
            continue;
        }

        const Usage u = usageFor(c);
        const std::string category = categoryName(c);

        if (passesEightyPercent(
                static_cast<long double>(u.units),
                static_cast<long double>(q->maxUnits))) {
            printWarning(
                category + " unit quota is above 80% (" +
                std::to_string(u.units) + "/" +
                std::to_string(q->maxUnits) + " units used)"
            );
        }

        if (passesEightyPercent(
                static_cast<long double>(u.spent.minorUnits()),
                static_cast<long double>(q->maxSpend.minorUnits()))) {
            printWarning(
                category + " spend quota is above 80% (" +
                u.spent.toString() + "/" +
                q->maxSpend.toString() + " spent)"
            );
        }

        if (q->maxTitles &&
            passesEightyPercent(
                static_cast<long double>(u.titles),
                static_cast<long double>(*q->maxTitles))) {
            printWarning(
                category + " title quota is above 80% (" +
                std::to_string(u.titles) + "/" +
                std::to_string(*q->maxTitles) + " titles used)"
            );
        }
    }

}

}  // namespace bookmgmt
