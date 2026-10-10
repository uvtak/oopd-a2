// Demo: builds a small catalog, sets a budget with per-category quotas,
// and runs a batch of purchase requests through the acquisition manager.

#include <iostream>
#include <stdexcept>

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
    std::cout << "10 copies of M001 = " << acq.quote("M001", 10) << "  (postage included, 10% bulk discount)\n";
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


/*
 * Q8: Limit the number of different titles bought per category.
 */
std::cout << "\n=== Title Quota Demo ===\n";

Catalog titleCatalog;

titleCatalog.emplace<Book>(
    "Q8B1", "Clean Code",
    std::vector<std::string>{"Author"},
    "ISBN-Q8-1", "Publisher", 2026, Money::of(100)
);

titleCatalog.emplace<Book>(
    "Q8B2", "Design Patterns",
    std::vector<std::string>{"Author"},
    "ISBN-Q8-2", "Publisher", 2026, Money::of(100)
);

titleCatalog.emplace<Book>(
    "Q8B3", "Effective C++",
    std::vector<std::string>{"Author"},
    "ISBN-Q8-3", "Publisher", 2026, Money::of(100)
);

// At most 2 different titles in the Book category.
Budget titleBudget(Money::of(1000));
titleBudget.setQuota(
    ResourceCategory::Book,
    {10, Money::of(1000), 2}
);

AcquisitionManager titleAcq(titleCatalog, titleBudget);

// Buying the same title twice counts as one distinct title.
titleAcq.purchase("Q8B1", 1);
titleAcq.purchase("Q8B1", 1);

// A second distinct title is allowed.
titleAcq.purchase("Q8B2", 1);

std::string titleReason;
const bool thirdTitleAllowed =
    titleAcq.canPurchase("Q8B3", 1, &titleReason);

std::cout << "Different titles used: "
          << titleBudget.usageFor(ResourceCategory::Book).titles
          << "/2\n";

std::cout << "Can buy the third different title? "
          << (thirdTitleAllowed ? "Yes" : "No") << "\n";

if (!thirdTitleAllowed) {
    std::cout << "Reason: " << titleReason << "\n";
}

titleBudget.print(std::cout);


    std::cout << "\n=== Order Cancellation Demo ===\n";

    Catalog cancelCatalog;

    cancelCatalog.emplace<Book>(
        "C001", "Cancellation Demo Book",
        std::vector<std::string>{"Author"},
        "ISBN-C001", "Publisher", 2026, Money::of(100)
    );

    Budget cancelBudget(Money::of(1000));
    cancelBudget.setQuota(
        ResourceCategory::Book,
        {5, Money::of(1000), 2}
    );

    AcquisitionManager cancelAcq(cancelCatalog, cancelBudget);

    const int cancelOrderNo = cancelAcq.purchase("C001", 2).orderNo;

    cancelAcq.cancelOrder(cancelOrderNo);
    cancelAcq.printReport(std::cout);

    std::cout << "Budget spent after cancellation: "
              << cancelBudget.spent() << "\n";

    std::cout << "Holdings after cancellation: "
              << cancelCatalog.holdings("C001") << " copies\n";

    std::cout << "Book titles used after cancellation: "
              << cancelBudget.usageFor(ResourceCategory::Book).titles
              << "/2\n";

    std::cout << "\n=== Q10 Department Budget Demo ===\n";

    Catalog departmentCatalog;

    departmentCatalog.emplace<Book>(
        "D001", "Computer Science Book",
        std::vector<std::string>{"Author"},
        "ISBN-D001", "Publisher", 2026, Money::of(100)
    );

    departmentCatalog.emplace<Book>(
        "D002", "Physics Book",
        std::vector<std::string>{"Author"},
        "ISBN-D002", "Publisher", 2026, Money::of(100)
    );

    Budget defaultBudget(Money::of(1000));

    Budget csBudget(Money::of(300));
    csBudget.setQuota(
        ResourceCategory::Book,
        {5, Money::of(200), 2}
    );

    Budget physicsBudget(Money::of(500));
    physicsBudget.setQuota(
        ResourceCategory::Book,
        {5, Money::of(400), 2}
    );

    AcquisitionManager departmentAcq(departmentCatalog, defaultBudget);

    departmentAcq.registerDepartment("Computer Science", csBudget);
    departmentAcq.registerDepartment("Physics", physicsBudget);

    departmentAcq.processBatch({
        {"D001", 2, "Computer Science"},
        {"D002", 2, "Physics"},
        {"D001", 1, "Computer Science"},
        {"D002", 1, "Biology"}
    });

    departmentAcq.printReport(std::cout);

    std::cout << "\nComputer Science budget:\n";
    csBudget.print(std::cout);

    std::cout << "\nPhysics budget:\n";
    physicsBudget.print(std::cout);

    std::cout << "\nDefault budget:\n";
    defaultBudget.print(std::cout);


    std::cout << "\n=== Q11 Year-end Budget Rollover ===\n";

    Budget currentYearBudget(Money::of(10000));

    currentYearBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(8000), 2}
    );

    currentYearBudget.commit(
        ResourceCategory::Book,
        7,
        Money::of(7000),
        "Current-year books"
    );

    std::cout << "Current year's budget:\n";
    currentYearBudget.print(std::cout);

    Budget nextYearBudget =
        currentYearBudget.rolloverToNextYear(
            Money::of(12000),
            50
        );

    std::cout
        << "\nNext year's budget "
        << "(base 12000 + 50% of unspent amount):\n";

    nextYearBudget.print(std::cout);

    std::cout << "\n=== Q12 Quota Warnings ===\n";

    Budget warningBudget(Money::of(5000));

    warningBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1000), 3}
    );

    warningBudget.commit(
        ResourceCategory::Book, 3,
        Money::of(300), "Clean Code"
    );

    warningBudget.commit(
        ResourceCategory::Book, 3,
        Money::of(250), "Design Patterns"
    );

    warningBudget.commit(
        ResourceCategory::Book, 3,
        Money::of(300), "Effective C++"
    );

    warningBudget.print(std::cout);
std::cout
    << "\n=== Official PDF Q11: All-or-nothing Batch ===\n";

Catalog atomicCatalog;

atomicCatalog.emplace<Book>(
    "AB1", "First Atomic Book",
    std::vector<std::string>{"Author"},
    "ISBN-AB1", "Publisher", 2026, Money::of(100)
);
atomicCatalog.emplace<Book>(
    "AB2", "Second Atomic Book",
    std::vector<std::string>{"Author"},
    "ISBN-AB2", "Publisher", 2026, Money::of(100)
);

Budget atomicBudget(Money::of(1000));
atomicBudget.setQuota(
    ResourceCategory::Book,
    {10, Money::of(250)}
);

AcquisitionManager atomicAcq(
    atomicCatalog, atomicBudget
);

auto atomicResults = atomicAcq.processBatch(
    {{"AB1", 1}, {"AB2", 2}},
    true
);

atomicAcq.printReport(std::cout);

std::cout
    << "Budget spent after batch: "
    << atomicBudget.spent() << "\n";

std::cout
    << "First book holdings: "
    << atomicCatalog.holdings("AB1") << "\n";

std::cout
    << "Second book holdings: "
    << atomicCatalog.holdings("AB2") << "\n";
    // Official PDF Q12: Multiple vendors and cheapest-price selection.
std::cout << "\n=== Official PDF Q12: Vendor Selection ===\n";

Catalog vendorCatalog;

vendorCatalog.emplace<Book>(
    "VB1", "Vendor Demo Book",
    std::vector<std::string>{"Author"},
    "ISBN-VB1", "Publisher", 2026, Money::of(100)
);

vendorCatalog.emplace<ElectronicResource>(
    "VR1", "Vendor Demo Database",
    "Publisher", 2026, Money::of(10),
    "https://vendor.example",
    LicenseModel::AnnualSubscription,
    Money::of(50)
);

Budget vendorBudget(Money::of(10000));
AcquisitionManager vendorAcq(vendorCatalog, vendorBudget);

vendorAcq.addVendorOffer("VB1", "Campus Books", Money::of(120));
vendorAcq.addVendorOffer("VB1", "Budget Books", Money::of(80));
vendorAcq.addVendorOffer("VB1", "City Books", Money::of(95));

vendorAcq.addVendorOffer("VR1", "Digital Source", Money::of(12));
vendorAcq.addVendorOffer("VR1", "EduAccess", Money::of(8));

std::cout << "Book vendor offers:\n";

for (const auto& offer : vendorAcq.vendorOffersFor("VB1")) {
    std::cout << "  " << offer.vendorName
              << ": " << offer.unitPrice << " per copy\n";
}

std::cout << "Cheapest quote for 2 books: "
          << vendorAcq.quote("VB1", 2) << "\n";

vendorAcq.processBatch({
    {"VB1", 2},
    {"VR1", 3}
});

vendorAcq.printReport(std::cout);

std::cout << "\nBudget after vendor purchases:\n";
vendorBudget.print(std::cout);
// Official PDF Q13: Catalogue searches.
std::cout << "\n=== Official PDF Q13: Catalogue Searches ===\n";

Catalog searchCatalog;

searchCatalog.emplace<Book>(
    "S-B1", "Algorithms",
    std::vector<std::string>{"Ada Lovelace", "Alan Turing"},
    "ISBN-101", "Publisher", 2015, Money::of(100)
);

searchCatalog.emplace<EBook>(
    "S-E1", "Digital Systems",
    std::vector<std::string>{"Ada Lovelace"},
    "ISBN-102", "Publisher", 2020, Money::of(20),
    "https://ebooks.example",
    LicenseModel::AnnualSubscription,
    Money{}, "EPUB", false
);

searchCatalog.emplace<Journal>(
    "S-J1", "Computing Journal",
    "ISSN-201", 12, "Publisher", 2018, Money::of(200)
);

std::cout << "\nSearch by author: Ada\n";
for (const Resource* resource : searchCatalog.searchAuthor("Ada")) {
    std::cout << "  " << resource->summary() << "\n";
}

std::cout << "\nSearch by ISBN: ISBN-102\n";
for (const Resource* resource :
     searchCatalog.searchISBNISSN("ISBN-102")) {
    std::cout << "  " << resource->summary() << "\n";
}

std::cout << "\nSearch by ISSN: ISSN-201\n";
for (const Resource* resource :
     searchCatalog.searchISBNISSN("ISSN-201")) {
    std::cout << "  " << resource->summary() << "\n";
}

std::cout << "\nSearch publication years: 2015-2018\n";
for (const Resource* resource :
     searchCatalog.searchYearRange(2015, 2018)) {
    std::cout << "  " << resource->summary() << "\n";
}
// Official PDF Q14: Lending and licensed electronic sessions.
std::cout << "\n=== Official PDF Q14: Lending ===\n";

Catalog lendingCatalog;

lendingCatalog.emplace<Book>(
    "L-B1", "Lending Demo Book",
    std::vector<std::string>{"Author"},
    "ISBN-LB1", "Publisher", 2026, Money::of(100)
);

lendingCatalog.emplace<ElectronicResource>(
    "L-R1", "Lending Demo Database",
    "Publisher", 2026, Money::of(20),
    "https://lending.example",
    LicenseModel::AnnualSubscription,
    Money{}
);

// The library owns three book copies and two database seats.
lendingCatalog.addHoldings("L-B1", 3);
lendingCatalog.addHoldings("L-R1", 2);

LendingManager lending(lendingCatalog);

std::cout << "Available book copies initially: "
          << lending.availableCopies("L-B1") << "\n";

lending.borrowCopies("P001", "L-B1", 2);
lending.borrowCopies("P002", "L-B1", 1);

std::cout << "P001 borrowed: "
          << lending.borrowedCopies("P001", "L-B1")
          << " copies\n";

std::cout << "Available book copies after borrowing: "
          << lending.availableCopies("L-B1") << "\n";

lending.returnCopies("P001", "L-B1", 1);

std::cout << "P001 borrowed after returning one: "
          << lending.borrowedCopies("P001", "L-B1")
          << " copies\n";

std::cout << "Available book copies after return: "
          << lending.availableCopies("L-B1") << "\n";

std::cout << "\nAvailable database seats initially: "
          << lending.availableSeats("L-R1") << "\n";

lending.openSession("P001", "L-R1");
lending.openSession("P002", "L-R1");

std::cout << "P001 active sessions: "
          << lending.activeSessions("P001", "L-R1") << "\n";

std::cout << "Available database seats after opening sessions: "
          << lending.availableSeats("L-R1") << "\n";

lending.closeSession("P001", "L-R1");

std::cout << "Available database seats after closing P001's session: "
          << lending.availableSeats("L-R1") << "\n";


    // Official PDF Q15: Currency-aware Money.
    std::cout << "\n=== Official PDF Q15: Currency Support ===\n";

    Catalog currencyCatalog;

    currencyCatalog.emplace<Book>(
        "USD-B1", "USD Demo Book",
        std::vector<std::string>{"Author"},
        "ISBN-USD", "Publisher", 2026,
        Money::of(100, 0, "USD")
    );

    Budget currencyBudget(
        Money::of(1000, 0, "USD")
    );

    currencyBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(800, 0, "USD")}
    );

    AcquisitionManager currencyAcq(
        currencyCatalog, currencyBudget
    );

    currencyAcq.setTaxRates(10, 20);

    const auto& usdOrder = currencyAcq.purchase("USD-B1", 2);

    std::cout << "Purchase currency: "
              << usdOrder.cost.currencyCode() << "\n";

    std::cout << "Pre-tax cost: "
              << usdOrder.preTaxCost.currencyCode() << " "
              << usdOrder.preTaxCost << "\n";

    std::cout << "Cost after 10% tax: "
              << usdOrder.cost.currencyCode() << " "
              << usdOrder.cost << "\n";

    std::cout << "Budget spent: "
              << currencyBudget.spent().currencyCode() << " "
              << currencyBudget.spent() << "\n";

    try {
        (void)(Money::of(1, 0, "USD")
               + Money::of(1, 0, "INR"));
    } catch (const std::invalid_argument& error) {
        std::cout << "Mixed currencies rejected: "
                  << error.what() << "\n";
    }

    std::cout << "Currency-aware acquisition report:\n";
    currencyAcq.printReport(std::cout);



return 0;
}
