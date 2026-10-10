#include "bookmgmt/Lending.h"

#include <stdexcept>

namespace bookmgmt {

LendingManager::LendingManager(const Catalog& catalog)
    : catalog_(catalog) {}

void LendingManager::validatePatron(
    const std::string& patronId
) const {
    if (patronId.empty()) {
        throw std::invalid_argument(
            "patron ID must not be empty"
        );
    }
}

int LendingManager::totalForResource(
    const std::map<PatronResourceKey, int>& records,
    const std::string& resourceId
) const {
    int total = 0;

    for (const auto& entry : records) {
        if (entry.first.second == resourceId) {
            total += entry.second;
        }
    }

    return total;
}

int LendingManager::borrowedCopies(
    const std::string& patronId,
    const std::string& resourceId
) const {
    const auto it = borrowed_.find({patronId, resourceId});

    return it == borrowed_.end() ? 0 : it->second;
}

int LendingManager::activeSessions(
    const std::string& patronId,
    const std::string& resourceId
) const {
    const auto it = sessions_.find({patronId, resourceId});

    return it == sessions_.end() ? 0 : it->second;
}

int LendingManager::availableCopies(
    const std::string& resourceId
) const {
    const Resource& resource = catalog_.get(resourceId);

    if (resource.isDigital()) {
        throw std::invalid_argument(
            "electronic resources use sessions, not copy loans"
        );
    }

    const int available =
        catalog_.holdings(resourceId) -
        totalForResource(borrowed_, resourceId);

    return available > 0 ? available : 0;
}

int LendingManager::availableSeats(
    const std::string& resourceId
) const {
    const Resource& resource = catalog_.get(resourceId);

    if (!resource.isDigital()) {
        throw std::invalid_argument(
            "print resources use copy loans, not sessions"
        );
    }

    const int available =
        catalog_.holdings(resourceId) -
        totalForResource(sessions_, resourceId);

    return available > 0 ? available : 0;
}

void LendingManager::borrowCopies(
    const std::string& patronId,
    const std::string& resourceId,
    int copies
) {
    validatePatron(patronId);

    if (copies <= 0) {
        throw std::invalid_argument(
            "number of copies must be positive"
        );
    }

    const Resource& resource = catalog_.get(resourceId);

    if (resource.isDigital()) {
        throw std::invalid_argument(
            "electronic resources must use openSession()"
        );
    }

    if (copies > availableCopies(resourceId)) {
        throw std::invalid_argument(
            "not enough available copies"
        );
    }

    borrowed_[{patronId, resourceId}] += copies;
}

void LendingManager::returnCopies(
    const std::string& patronId,
    const std::string& resourceId,
    int copies
) {
    validatePatron(patronId);

    if (copies <= 0) {
        throw std::invalid_argument(
            "number of copies must be positive"
        );
    }

    const Resource& resource = catalog_.get(resourceId);

    if (resource.isDigital()) {
        throw std::invalid_argument(
            "electronic resources do not use copy returns"
        );
    }

    const PatronResourceKey key{patronId, resourceId};
    const auto it = borrowed_.find(key);

    if (it == borrowed_.end() || it->second < copies) {
        throw std::invalid_argument(
            "patron has not borrowed that many copies"
        );
    }

    it->second -= copies;

    if (it->second == 0) {
        borrowed_.erase(it);
    }
}

void LendingManager::openSession(
    const std::string& patronId,
    const std::string& resourceId
) {
    validatePatron(patronId);

    const Resource& resource = catalog_.get(resourceId);

    if (!resource.isDigital()) {
        throw std::invalid_argument(
            "print resources do not support electronic sessions"
        );
    }

    if (availableSeats(resourceId) <= 0) {
        throw std::invalid_argument(
            "no licensed seats available"
        );
    }

    ++sessions_[{patronId, resourceId}];
}

void LendingManager::closeSession(
    const std::string& patronId,
    const std::string& resourceId
) {
    validatePatron(patronId);

    const Resource& resource = catalog_.get(resourceId);

    if (!resource.isDigital()) {
        throw std::invalid_argument(
            "print resources do not support electronic sessions"
        );
    }

    const PatronResourceKey key{patronId, resourceId};
    const auto it = sessions_.find(key);

    if (it == sessions_.end() || it->second <= 0) {
        throw std::invalid_argument(
            "patron has no active session for this resource"
        );
    }

    --it->second;

    if (it->second == 0) {
        sessions_.erase(it);
    }
}

}  // namespace bookmgmt