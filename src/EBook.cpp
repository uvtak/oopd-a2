#include "bookmgmt/EBook.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

EBook::EBook(std::string id, std::string title,
             std::vector<std::string> authors,
             std::string isbn, std::string publisher,
             int year, Money pricePerSeat, std::string accessUrl,
             LicenseModel license, Money platformFee,
             std::string format, bool drmProtected)
    : ElectronicResource(std::move(id), std::move(title),
                         std::move(publisher), year, pricePerSeat,
                         std::move(accessUrl), license, platformFee),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      format_(std::move(format)),
      drmProtected_(drmProtected) {
    if (format_ != "PDF" && format_ != "EPUB" && format_ != "HTML") {
        throw std::invalid_argument("file format must be PDF, EPUB or HTML");
    }
}

void EBook::printDetails(std::ostream& os) const {
    ElectronicResource::printDetails(os);

    os << "  authors: " << joinAuthors(authors_) << "\n"
       << "  isbn: " << isbn_ << "\n"
       << "  format: " << format_ << "\n"
       << "  drm protected: " << (drmProtected_ ? "true" : "false") << "\n";
}

/*
 * Design note:
 * EBook and Book both contain authors and ISBN-related information.
 * This duplication could be reduced by introducing a shared base class
 * for resources that have bibliographic information, or by using
 * composition with a shared bibliographic-info class.
 */

}  // namespace bookmgmt