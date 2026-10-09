// Demo: builds a small catalog, sets a budget with per-category quotas,
// and runs a batch of purchase requests through the acquisition manager.

#include <iostream>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

int main() {
    Catalog catalog;

    catalog.emplace<Book>("B001", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                          "978-0132350884", "Prentice Hall", 2008, Money::of(450));
    catalog.emplace<Book>("B002", "The C++ Programming Language",
                          std::vector<std::string>{"Bjarne Stroustrup"}, "978-0321563842",
                          "Addison-Wesley", 2013, Money::of(1200), 4, Binding::Hardcover);
    catalog.emplace<ElectronicResource>("R001", "IEEE Xplore Digital Library", "IEEE", 2026,
                                        Money::of(150), "https://ieeexplore.example",
                                        LicenseModel::AnnualSubscription, Money::of(2000));
    catalog.emplace<ElectronicResource>("R002", "MATLAB Campus Licence", "MathWorks", 2026,
                                        Money::of(400), "https://licensing.example/matlab",
                                        LicenseModel::Perpetual);
    catalog.emplace<Journal>("J001", "Nature", "1476-4687", 52,
                          "Springer Nature", 2026, Money::of(500), 2);
    catalog.emplace<Magazine>(
    "M001",
    "TIME Magazine",
    "0040-781X",
    52,
    "Time USA",
    2026,
    Money::of(500),
    1,
    Money::of(20)
);
    catalog.emplace<AudioBook>(
    "A001",
    "The Pragmatic Programmer",
    "John Doe",
    540,
    "Tech Publisher",
    2026,
    Money::of(100),
    "https://audio.example/pragmatic",
    LicenseModel::AnnualSubscription,
    Money::of(50)
);

catalog.emplace<Thesis>(
    "T001",
    "Efficient Graph Algorithms",
    "IIIT Delhi",
    "M.Tech CSE",
    "Dr. Professor",
    2026
);
    catalog.emplace<EBook>(
    "E001",
    "Clean Code EBook",
    std::vector<std::string>{"Robert C. Martin"},
    "978-0132350884",
    "Prentice Hall",
    2008,
    Money::of(100),
    "https://ebooks.example/cleancode",
    LicenseModel::AnnualSubscription,
    Money::of(500),
    "EPUB",
    true
    
);
    std::cout << "=== Catalog ===\n";
    for (const Resource* r : catalog.all()) std::cout << r->summary() << "\n";

    std::cout << "\n=== Details of R001 ===\n" << catalog.get("R001");
    std::cout << "\n=== Details of J001 ===\n" << catalog.get("J001");
    std::cout << "\n=== Details of M001 ===\n" << catalog.get("M001");
    std::cout << "\n=== Details of E001 ===\n" << catalog.get("E001");
    std::cout << "\n=== Details of A001 ===\n" << catalog.get("A001");
    std::cout << "\n=== Details of T001 ===\n" << catalog.get("T001");
    Budget budget(Money::of(22000));
    budget.setQuota(ResourceCategory::Book, {10, Money::of(8000)});
    budget.setQuota(ResourceCategory::ElectronicResource, {40, Money::of(12000)});
    budget.setQuota(ResourceCategory::Journal, {5, Money::of(6000)});
    budget.setQuota(ResourceCategory::EBook, {10, Money::of(5000)});
    AcquisitionManager acq(catalog, budget);

    std::cout << "\n=== Quotes ===\n";
    std::cout << "5 copies of B002  = " << acq.quote("B002", 5) << "\n";
    std::cout << "1 copy of B002   = " << acq.quote("B002", 1) << "  (Hardcover, 20% surcharge)\n";
    std::cout << "20 seats of R001  = " << acq.quote("R001", 20) << "  (incl. platform fee)\n";
    std::cout << "2 copies of J001  = " << acq.quote("J001", 2) << "  (2-year subscription)\n";
    std::cout << "2 copies of M001  = " << acq.quote("M001", 2) << "  (includes postage)\n";
    std::cout << "3 seats of E001   = " << acq.quote("E001", 3) << "  (incl. platform fee)\n";
    std::cout << "10 copies of B001 = " << acq.quote("B001", 10) << "  (10% bulk discount)\n";
    std::cout << "10 copies of J001 = " << acq.quote("J001", 10) << "  (10% bulk discount)\n";
    std::cout << "50 seats of R001  = " << acq.quote("R001", 50) << "  (full price)\n";
    std::cout << "60 seats of R001  = " << acq.quote("R001", 60) << "  (seats 51+ at half price)\n";
    std::cout << "2 seats of A001   = " << acq.quote("A001", 2) << "  (incl. platform fee)\n";
    std::cout << "2 copies of T001  = " << acq.quote("T001", 2) << "  (thesis)\n";
    acq.processBatch({
        {"B001", 4},   // 1800  ok
        {"B002", 5},   // 7200  rejected: Hardcover price exceeds book spend quota
        {"B001", 1},   // 450   ok
        {"R001", 20},  // 5000  ok
        {"R002", 25},  // 10000 rejected: e-resource unit quota (20 seats left)
        {"R002", 15},  // 6000  ok  -> e-resource spend 11000
        {"R002", 5},   // 2000  rejected: e-resource spend quota (1000 left)
        {"X999", 1},   // rejected: unknown id
        {"J001", 2},   // 2000  ok
    });

    std::cout << "\n=== Acquisition report ===\n";
    acq.printReport(std::cout);

    std::cout << "\n=== Budget ===\n";
    budget.print(std::cout);

    std::cout << "\n=== Holdings ===\n";
    for (const Resource* r : catalog.all())
        std::cout << "  " << r->id() << ": " << catalog.holdings(r->id())
                  << (r->isDigital() ? " seats" : " copies") << "\n";

    // Direct purchase: errors are reported with exceptions
std::cout << "\n=== Direct purchase that breaks a quota ===\n";
try {
    acq.purchase("B002", 5);
} catch (const QuotaExceededError& e) {
    std::cout << "QuotaExceededError: " << e.what() << "\n";
}

/*
 * Q6: Tax demonstration.
 *
 * Print resources use 10% tax.
 * Electronic resources use 20% tax.
 *
 * Budget/quota checks use the post-tax cost.
 */
std::cout << "\n=== Tax Demo ===\n";

Catalog taxCatalog;

taxCatalog.emplace<Book>(
    "TB1",
    "Taxed Book",
    std::vector<std::string>{"Author"},
    "ISBN-TAX",
    "Publisher",
    2026,
    Money::of(100)
);

taxCatalog.emplace<ElectronicResource>(
    "TR1",
    "Taxed Database",
    "Publisher",
    2026,
    Money::of(100),
    "https://tax.example",
    LicenseModel::AnnualSubscription,
    Money::of(50)
);

Budget taxBudget(Money::of(1000));

taxBudget.setQuota(
    ResourceCategory::Book,
    {10, Money::of(300)}
);

taxBudget.setQuota(
    ResourceCategory::ElectronicResource,
    {10, Money::of(400)}
);

AcquisitionManager taxAcq(taxCatalog, taxBudget);

taxAcq.setTaxRates(10, 20);

std::cout << "Print tax rate: "
          << taxAcq.printTaxPercent() << "%\n";

std::cout << "Electronic tax rate: "
          << taxAcq.electronicTaxPercent() << "%\n";

/*
 * Book:
 * pre-tax  = 100 × 2 = 200
 * tax      = 20
 * post-tax = 220
 */
taxAcq.purchase("TB1", 2);

/*
 * Electronic resource:
 * pre-tax  = 50 platform fee + (100 × 2) = 250
 * tax      = 50
 * post-tax = 300
 */
taxAcq.purchase("TR1", 2);

std::cout << "\n=== Tax Acquisition Report ===\n";
taxAcq.printReport(std::cout);

std::cout << "\n=== Tax Budget ===\n";
taxBudget.print(std::cout);

return 0;
}
