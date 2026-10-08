#include "bookmgmt/AudioBook.h"

#include <ostream>
#include <utility>

namespace bookmgmt {

AudioBook::AudioBook(std::string id, std::string title,
                     std::string narrator, int durationMinutes,
                     std::string publisher, int year,
                     Money unitPrice, std::string accessUrl,
                     LicenseModel license, Money platformFee)
    : ElectronicResource(std::move(id), std::move(title),
                         std::move(publisher), year, unitPrice,
                         std::move(accessUrl), license, platformFee),
      narrator_(std::move(narrator)),
      durationMinutes_(durationMinutes) {
}

void AudioBook::printDetails(std::ostream& os) const {
    ElectronicResource::printDetails(os);

    os << "  narrator: " << narrator_ << "\n"
       << "  duration minutes: " << durationMinutes_ << "\n";
}

}  // namespace bookmgmt
