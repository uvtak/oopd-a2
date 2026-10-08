#pragma once

#include <string>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

class Thesis : public Resource {
public:
    Thesis(std::string id, std::string title,
           std::string university, std::string degree,
           std::string supervisor, int year,
           Money unitPrice = Money{});

    const std::string& university() const { return university_; }
    const std::string& degree() const { return degree_; }
    const std::string& supervisor() const { return supervisor_; }

    ResourceCategory category() const override {
        return ResourceCategory::Thesis;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string university_;
    std::string degree_;
    std::string supervisor_;
};

/*
 * Design choice:
 * Thesis inherits directly from Resource because it is a catalogue
 * resource but does not naturally have the book-specific properties
 * of Book or the digital licensing model of ElectronicResource.
 * A thesis can also be free of cost by using a zero Money unit price.
 */

}  // namespace bookmgmt