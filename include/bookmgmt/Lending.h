#pragma once

#include <map>
#include <string>
#include <utility>

#include "bookmgmt/Catalog.h"

namespace bookmgmt {

// Official PDF Q14: Borrow/return print copies and
// open/close sessions on electronic resources.
class LendingManager {
public:
    explicit LendingManager(const Catalog& catalog);

    void borrowCopies(
        const std::string& patronId,
        const std::string& resourceId,
        int copies = 1
    );

    void returnCopies(
        const std::string& patronId,
        const std::string& resourceId,
        int copies = 1
    );

    void openSession(
        const std::string& patronId,
        const std::string& resourceId
    );

    void closeSession(
        const std::string& patronId,
        const std::string& resourceId
    );

    int borrowedCopies(
        const std::string& patronId,
        const std::string& resourceId
    ) const;

    int activeSessions(
        const std::string& patronId,
        const std::string& resourceId
    ) const;

    int availableCopies(const std::string& resourceId) const;
    int availableSeats(const std::string& resourceId) const;

private:
    using PatronResourceKey =
        std::pair<std::string, std::string>;

    void validatePatron(const std::string& patronId) const;

    int totalForResource(
        const std::map<PatronResourceKey, int>& records,
        const std::string& resourceId
    ) const;

    const Catalog& catalog_;

    // Key: {patron ID, resource ID}
    std::map<PatronResourceKey, int> borrowed_;
    std::map<PatronResourceKey, int> sessions_;
};

}  // namespace bookmgmt