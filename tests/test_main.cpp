// Minimal self-contained test runner (no external framework needed).

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failures;                                                        \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond \
                      << "\n";                                                   \
        }                                                                        \
    } while (0)

#define CHECK_THROWS(expr, ExType)            \
    do {                                      \
        bool thrown_ = false;                 \
        try {                                 \
            (void)(expr);                     \
        } catch (const ExType&) {             \
            thrown_ = true;                   \
        } catch (...) {                       \
        }                                     \
        CHECK(thrown_ && "expected " #ExType); \
    } while (0)

static void testMoney() {
    CHECK(Money::of(12, 5).toString() == "12.05");
    CHECK(Money::of(-3, 50).toString() == "-3.50");
    CHECK(Money::fromMinor(7).toString() == "0.07");
    CHECK(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    CHECK(Money::of(3) * 4 == Money::of(12));
    CHECK(Money::of(1) < Money::of(2));
    CHECK_THROWS(Money::of(1, 100), std::invalid_argument);
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    CHECK(b.category() == ResourceCategory::Book);
    CHECK(!b.isDigital());
    CHECK(b.costFor(3) == Money::of(300));
    CHECK_THROWS(b.costFor(0), std::invalid_argument);
    CHECK(joinAuthors(b.authors()) == "A, B and C");

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    CHECK(e.isDigital());
    CHECK(e.costFor(5) == Money::of(150));

    CHECK(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference
    const Resource& r = e;
    CHECK(r.costFor(1) == Money::of(110));
    std::ostringstream os;
    os << r;
    CHECK(os.str().find("platform fee: 100.00") != std::string::npos);

    CHECK_THROWS(Book("", "T", {}, "", "", 2000, Money::of(1)), std::invalid_argument);
    CHECK_THROWS(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1)),
                 std::invalid_argument);
}
static void testJournal() {
    Journal j("J1", "Nature", "2049-3630", 12, "Springer", 2026,
              Money::of(500), 2);

    CHECK(j.category() == ResourceCategory::Journal);
    CHECK(categoryName(j.category()) == std::string("Journal"));
    CHECK(!j.isDigital());

    CHECK(j.issn() == "2049-3630");
    CHECK(j.issuesPerYear() == 12);
    CHECK(j.subscriptionYears() == 2);

    // 500 × 3 copies × 2 years = 3000
    CHECK(j.costFor(3) == Money::of(3000));

    CHECK_THROWS(j.costFor(0), std::invalid_argument);

    CHECK_THROWS(
        Journal("J2", "Science", "1234-5678", 12, "Publisher",
                2026, Money::of(500), 0),
        std::invalid_argument
    );

    std::ostringstream os;
    j.print(os);

    CHECK(os.str().find("issn: 2049-3630") != std::string::npos);
    CHECK(os.str().find("issues per year: 12") != std::string::npos);
    CHECK(os.str().find("subscription years: 2") != std::string::npos);

    Budget b(Money::of(5000));
    b.setQuota(ResourceCategory::Journal, {5, Money::of(4000)});

    CHECK(b.quotaFor(ResourceCategory::Journal).has_value());

    std::ostringstream budgetOut;
    b.print(budgetOut);

    CHECK(budgetOut.str().find("Journal") != std::string::npos);
}
static void testMagazine() {
    Magazine m(
        "M1",
        "TIME Magazine",
        "0040-781X",
        52,
        "Time USA",
        2026,
        Money::of(500),
        2,
        Money::of(10)
    );

    CHECK(m.category() == ResourceCategory::Journal);
    CHECK(!m.isDigital());

    CHECK(m.issn() == "0040-781X");
    CHECK(m.issuesPerYear() == 52);
    CHECK(m.subscriptionYears() == 2);
    CHECK(m.postagePerIssue() == Money::of(10));

    // Subscription = 500 × 2 × 2 = 2000
    // Postage = 10 × 52 × 2 × 2 = 2080
    // Total = 4080
    CHECK(m.costFor(2) == Money::of(4080));
    // 10 copies:
    // Subscription = 500 × 10 × 2 = 10000
    // Postage = 10 × 52 × 10 × 2 = 10400
    // Combined total = 20400
    // 10% bulk discount = 18360

    CHECK(m.costFor(10) == Money::of(18360));

    CHECK_THROWS(m.costFor(0), std::invalid_argument);

    CHECK_THROWS(
        Magazine("M2", "Bad Magazine", "1234-5678",
                 12, "Publisher", 2026,
                 Money::of(100), 1, Money::of(-1)),
        std::invalid_argument
    );

    std::ostringstream os;
    m.print(os);

    CHECK(os.str().find("issn: 0040-781X") != std::string::npos);
    CHECK(os.str().find("issues per year: 52") != std::string::npos);
    CHECK(os.str().find("subscription years: 2") != std::string::npos);
    CHECK(os.str().find("postage per issue: 10.00") != std::string::npos);
}
static void testEBook() {
    EBook e(
        "E1",
        "Clean Code EBook",
        {"Robert C. Martin"},
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

    CHECK(e.category() == ResourceCategory::EBook);
    CHECK(categoryName(e.category()) == std::string("EBook"));

    // EBook inherits digital behaviour from ElectronicResource.
    CHECK(e.isDigital());

    // Pricing must remain unchanged from ElectronicResource:
    // platform fee + price per seat × seats
    CHECK(e.costFor(3) == Money::of(800));

    CHECK(e.authors().size() == 1);
    CHECK(e.authors()[0] == "Robert C. Martin");
    CHECK(e.isbn() == "978-0132350884");
    CHECK(e.format() == "EPUB");
    CHECK(e.drmProtected());

    // All inherited and EBook-specific details should be printed.
    std::ostringstream os;
    e.print(os);

    CHECK(os.str().find("access url: https://ebooks.example/cleancode") !=
          std::string::npos);
    CHECK(os.str().find("platform fee: 500.00") != std::string::npos);
    CHECK(os.str().find("authors: Robert C. Martin") != std::string::npos);
    CHECK(os.str().find("isbn: 978-0132350884") != std::string::npos);
    CHECK(os.str().find("format: EPUB") != std::string::npos);
    CHECK(os.str().find("drm protected: true") != std::string::npos);

    // Only PDF, EPUB and HTML are allowed.
    CHECK_THROWS(
        EBook("E2", "Bad Format", {"A"}, "ISBN", "P", 2026,
               Money::of(100), "url",
               LicenseModel::AnnualSubscription, Money{},
               "DOCX", false),
        std::invalid_argument
    );

    // EBook must have its own budget quota.
    Budget b(Money::of(5000));
    b.setQuota(ResourceCategory::EBook, {10, Money::of(3000)});

    CHECK(b.quotaFor(ResourceCategory::EBook).has_value());

    std::ostringstream budgetOut;
    b.print(budgetOut);

    CHECK(budgetOut.str().find("EBook") != std::string::npos);
}
static void testAudioBookAndThesis() {
    AudioBook audio(
        "A1",
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

    CHECK(audio.category() == ResourceCategory::AudioBook);
    CHECK(categoryName(audio.category()) == std::string("AudioBook"));
    CHECK(audio.isDigital());

    CHECK(audio.narrator() == "John Doe");
    CHECK(audio.durationMinutes() == 540);

    // AudioBook inherits ElectronicResource pricing unchanged:
    // platform fee + unit price × seats
    CHECK(audio.costFor(3) == Money::of(350));

    std::ostringstream audioOut;
    audio.print(audioOut);

    CHECK(audioOut.str().find("access url: https://audio.example/pragmatic") !=
          std::string::npos);
    CHECK(audioOut.str().find("platform fee: 50.00") !=
          std::string::npos);
    CHECK(audioOut.str().find("narrator: John Doe") !=
          std::string::npos);
    CHECK(audioOut.str().find("duration minutes: 540") !=
          std::string::npos);


    Thesis thesis(
        "T1",
        "Efficient Graph Algorithms",
        "IIIT Delhi",
        "M.Tech CSE",
        "Dr. Professor",
        2026
    );

    CHECK(thesis.category() == ResourceCategory::Thesis);
    CHECK(categoryName(thesis.category()) == std::string("Thesis"));
    CHECK(!thesis.isDigital());

    CHECK(thesis.university() == "IIIT Delhi");
    CHECK(thesis.degree() == "M.Tech CSE");
    CHECK(thesis.supervisor() == "Dr. Professor");

    // Thesis can be free of cost.
    CHECK(thesis.unitPrice() == Money{});
    CHECK(thesis.costFor(3) == Money{});

    std::ostringstream thesisOut;
    thesis.print(thesisOut);

    CHECK(thesisOut.str().find("university: IIIT Delhi") !=
          std::string::npos);
    CHECK(thesisOut.str().find("degree: M.Tech CSE") !=
          std::string::npos);
    CHECK(thesisOut.str().find("supervisor: Dr. Professor") !=
          std::string::npos);
}
static void testHardcoverPricing() {
    Book paperback(
        "BP1", "Paperback Book",
        {"Author"}, "ISBN1", "Publisher",
        2026, Money::of(100),
        1, Binding::Paperback
    );

    Book hardcover(
        "BH1", "Hardcover Book",
        {"Author"}, "ISBN2", "Publisher",
        2026, Money::of(100),
        1, Binding::Hardcover
    );

    CHECK(paperback.costFor(3) == Money::of(300));
    CHECK(hardcover.costFor(3) == Money::of(360));

    CHECK_THROWS(hardcover.costFor(0), std::invalid_argument);
}
static void testBulkDiscounts() {
    // Print item: 10% discount for 10 or more copies.
    Book paperback(
        "BP1", "Bulk Book",
        {"Author"}, "ISBN1", "Publisher",
        2026, Money::of(100),
        1, Binding::Paperback
    );

    CHECK(paperback.costFor(9) == Money::of(900));
    CHECK(paperback.costFor(10) == Money::of(900));
    CHECK(paperback.costFor(20) == Money::of(1800));

    // Hardcover price increase from Q4 + 10% bulk discount from Q5.
    Book hardcover(
        "BH1", "Bulk Hardcover",
        {"Author"}, "ISBN2", "Publisher",
        2026, Money::of(100),
        1, Binding::Hardcover
    );

    CHECK(hardcover.costFor(10) == Money::of(1080));

    // Journal is also a print item.
    Journal journal(
        "J1", "Journal",
        "1234-5678", 12,
        "Publisher", 2026,
        Money::of(50), 2
    );

    // 50 × 10 copies × 2 years = 1000, then 10% off = 900.
    CHECK(journal.costFor(9) == Money::of(900));
    CHECK(journal.costFor(10) == Money::of(900));

    // Electronic resource:
    // first 50 seats at full price, seats beyond 50 at half price.
    ElectronicResource er(
        "R1", "Database",
        "Publisher", 2026,
        Money::of(100), "https://example.com",
        LicenseModel::AnnualSubscription,
        Money::of(500)
    );

    CHECK(er.costFor(50) == Money::of(5500));
    CHECK(er.costFor(51) == Money::of(5550));
    CHECK(er.costFor(60) == Money::of(6000));
}
static void testTaxes() {
    Catalog c;

    c.emplace<Book>(
        "B1",
        "Taxed Book",
        std::vector<std::string>{"Author"},
        "ISBN",
        "Publisher",
        2026,
        Money::of(100)
    );

    c.emplace<ElectronicResource>(
        "R1",
        "Taxed Database",
        "Publisher",
        2026,
        Money::of(100),
        "https://example.com",
        LicenseModel::AnnualSubscription,
        Money::of(50)
    );

    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {10, Money::of(250)});
    b.setQuota(ResourceCategory::ElectronicResource, {10, Money::of(400)});

    AcquisitionManager acq(c, b);

    // 10% tax on print resources, 20% tax on electronic resources.
    acq.setTaxRates(10, 20);

    CHECK(acq.printTaxPercent() == 10);
    CHECK(acq.electronicTaxPercent() == 20);

    // Book:
    // pre-tax = 100
    // tax = 10
    // post-tax = 110
    CHECK(acq.canPurchase("B1", 2));

    std::string why;
    CHECK(!acq.canPurchase("B1", 3, &why));
    CHECK(why.find("spend quota") != std::string::npos);

    const auto& bookRecord = acq.purchase("B1", 2);

    CHECK(bookRecord.preTaxCost == Money::of(200));
    CHECK(bookRecord.cost == Money::of(220));

    // Electronic resource:
    // pre-tax = platform fee 50 + 100 × 2 = 250
    // tax = 20% of 250 = 50
    // post-tax = 300
    CHECK(acq.canPurchase("R1", 2));

    const auto& electronicRecord = acq.purchase("R1", 2);

    CHECK(electronicRecord.preTaxCost == Money::of(250));
    CHECK(electronicRecord.cost == Money::of(300));

    // Negative tax rates are invalid.
    CHECK_THROWS(acq.setTaxRates(-1, 10), std::invalid_argument);
    CHECK_THROWS(acq.setTaxRates(10, -1), std::invalid_argument);

    // Report should contain both pre-tax and post-tax values.
    std::ostringstream report;
    acq.printReport(report);

    CHECK(report.str().find("pre-tax 200.00") != std::string::npos);
    CHECK(report.str().find("post-tax 220.00") != std::string::npos);
    CHECK(report.str().find("pre-tax 250.00") != std::string::npos);
    CHECK(report.str().find("post-tax 300.00") != std::string::npos);
    CHECK(report.str().find("Total pre-tax: 450.00") != std::string::npos);
    CHECK(report.str().find("Total post-tax: 520.00") != std::string::npos);
    // Budget spent must use post-tax amounts.
    CHECK(b.spent() == Money::of(520));

    // Total spent must also use post-tax amounts.
    CHECK(acq.totalSpent() == Money::of(520));
}
static void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    CHECK(c.size() == 3);
    CHECK(c.contains("B1"));
    CHECK(c.find("nope") == nullptr);
    CHECK_THROWS(c.get("nope"), NotFoundError);
    CHECK_THROWS(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1)),
                 DuplicateIdError);

    CHECK(c.searchTitle("clean").size() == 2);
    CHECK(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);
    CHECK(c.where([](const Resource& r) { return r.isDigital(); }).size() == 1);

    CHECK(c.holdings("B1") == 0);
    c.addHoldings("B1", 3);
    CHECK(c.holdings("B1") == 3);
    CHECK_THROWS(c.addHoldings("B1", -5), std::invalid_argument);

    c.remove("R1");
    CHECK(c.size() == 2);
    CHECK_THROWS(c.remove("R1"), NotFoundError);
}

static void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});

    CHECK(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    CHECK(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());   // units
    CHECK(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());  // spend
    CHECK(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());  // overall
    CHECK(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());    // no quota

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    CHECK(b.spent() == Money::of(300));
    CHECK(*b.unitsRemaining(ResourceCategory::Book) == 1);
    CHECK(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    CHECK(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());

    CHECK_THROWS(b.commit(ResourceCategory::Book, 2, Money::of(10)), QuotaExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800)),
                 BudgetExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1)),
                 std::invalid_argument);
    CHECK(b.spent() == Money::of(300));  // failed commits changed nothing
}

static void testBudgetRollover() {
    Budget current(Money::of(10000));

    current.setQuota(
        ResourceCategory::Book,
        {10, Money::of(8000), 2}
    );

    current.commit(
        ResourceCategory::Book,
        7,
        Money::of(7000),
        "Current-year books"
    );

    CHECK(current.spent() == Money::of(7000));
    CHECK(current.remaining() == Money::of(3000));

    // 50% of 3000 is carried into the next year's base budget.
    Budget next =
        current.rolloverToNextYear(Money::of(12000), 50);

    CHECK(next.total() == Money::of(13500));
    CHECK(next.spent() == Money{});
    CHECK(next.remaining() == Money::of(13500));

    // The new year starts with fresh usage and no old quotas.
    CHECK(next.usageFor(ResourceCategory::Book).units == 0);
    CHECK(next.usageFor(ResourceCategory::Book).titles == 0);
    CHECK(!next.quotaFor(ResourceCategory::Book).has_value());

    // The original budget remains unchanged.
    CHECK(current.total() == Money::of(10000));
    CHECK(current.spent() == Money::of(7000));

    // 0% carries nothing; 100% carries all unspent funds.
    Budget noCarry =
        current.rolloverToNextYear(Money::of(12000), 0);
    CHECK(noCarry.total() == Money::of(12000));

    Budget fullCarry =
        current.rolloverToNextYear(Money::of(12000), 100);
    CHECK(fullCarry.total() == Money::of(15000));

    // Invalid percentages and a negative next-year allocation are rejected.
    CHECK_THROWS(
        current.rolloverToNextYear(Money::of(12000), -1),
        std::invalid_argument
    );

    CHECK_THROWS(
        current.rolloverToNextYear(Money::of(12000), 101),
        std::invalid_argument
    );

    CHECK_THROWS(
        current.rolloverToNextYear(Money::of(-1), 50),
        std::invalid_argument
    );
}

static void testBudgetWarnings() {
    Budget b(Money::of(5000));

    b.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1000), 3}
    );

    b.commit(
        ResourceCategory::Book, 3,
        Money::of(300), "Clean Code"
    );

    b.commit(
        ResourceCategory::Book, 3,
        Money::of(250), "Design Patterns"
    );

    b.commit(
        ResourceCategory::Book, 3,
        Money::of(300), "Effective C++"
    );

    std::ostringstream output;
    b.print(output);

    // 9/10 units = 90%.
    CHECK(output.str().find(
        "Book unit quota is above 80%"
    ) != std::string::npos);

    // 850/1000 spent = 85%.
    CHECK(output.str().find(
        "Book spend quota is above 80%"
    ) != std::string::npos);

    // 3/3 different titles = 100%.
    CHECK(output.str().find(
        "Book title quota is above 80%"
    ) != std::string::npos);

    // Exactly 80% must not trigger a warning.
    Budget exact(Money::of(2000));

    exact.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1000), 5}
    );

    exact.commit(
        ResourceCategory::Book, 8,
        Money::of(800), "Exactly Eighty"
    );

    std::ostringstream exactOutput;
    exact.print(exactOutput);

    CHECK(exactOutput.str().find("WARNING:") ==
          std::string::npos);
}

static void testTitleQuota() {
    Catalog c;

    c.emplace<Book>(
        "B1", "Clean Code",
        std::vector<std::string>{"Author"},
        "ISBN1", "Publisher", 2020, Money::of(100)
    );

    // Different ID, but the same title.
    c.emplace<Book>(
        "B2", "Clean Code",
        std::vector<std::string>{"Author"},
        "ISBN2", "Publisher", 2021, Money::of(100)
    );

    c.emplace<Book>(
        "B3", "The Pragmatic Programmer",
        std::vector<std::string>{"Author"},
        "ISBN3", "Publisher", 2022, Money::of(100)
    );

    c.emplace<Book>(
        "B4", "Design Patterns",
        std::vector<std::string>{"Author"},
        "ISBN4", "Publisher", 2023, Money::of(100)
    );

    Budget b(Money::of(1000));

    // Maximum 10 units, 1000 spending, and 2 different titles.
    b.setQuota(ResourceCategory::Book,
               {10, Money::of(1000), 2});

    AcquisitionManager acq(c, b);

    // First title is allowed.
    CHECK(acq.canPurchase("B1", 1));
    acq.purchase("B1", 1);

    CHECK(b.usageFor(ResourceCategory::Book).titles == 1);

    // Same title with a different ID should still be allowed.
    CHECK(acq.canPurchase("B2", 1));
    acq.purchase("B2", 1);

    CHECK(b.usageFor(ResourceCategory::Book).titles == 1);

    // Second different title is allowed.
    CHECK(acq.canPurchase("B3", 1));
    acq.purchase("B3", 1);

    CHECK(b.usageFor(ResourceCategory::Book).titles == 2);

    // Third different title must be rejected.
    std::string why;
    CHECK(!acq.canPurchase("B4", 1, &why));
    CHECK(why.find("title quota") != std::string::npos);

    auto results = acq.processBatch({{"B4", 1}});

    CHECK(results.size() == 1);
    CHECK(!results[0].approved);
    CHECK(results[0].reason.find("title quota") != std::string::npos);

    // Rejected purchase must not change budget, units, or holdings.
    CHECK(b.usageFor(ResourceCategory::Book).titles == 2);
    CHECK(b.usageFor(ResourceCategory::Book).units == 3);
    CHECK(b.spent() == Money::of(300));
    CHECK(c.holdings("B4") == 0);
}

static void testCancelOrder() {
    Catalog c;

    c.emplace<Book>(
        "B1", "Clean Code",
        std::vector<std::string>{"Author"},
        "ISBN1", "Publisher", 2020, Money::of(100)
    );

    c.emplace<Book>(
        "B2", "Design Patterns",
        std::vector<std::string>{"Author"},
        "ISBN2", "Publisher", 2021, Money::of(100)
    );

    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {10, Money::of(1000), 1});

    AcquisitionManager acq(c, b);

    const int originalOrderNo = acq.purchase("B1", 2).orderNo;

    auto rejected = acq.processBatch({{"B2", 1}});
    CHECK(rejected.size() == 1);
    CHECK(!rejected[0].approved);

    const auto& cancellation = acq.cancelOrder(originalOrderNo);

    CHECK(cancellation.cancellation);
    CHECK(cancellation.relatedOrderNo == originalOrderNo);
    CHECK(cancellation.cost == Money::of(-200));
    CHECK(cancellation.preTaxCost == Money::of(-200));

    CHECK(acq.history().size() == 3);
    CHECK(acq.history()[0].approved);
    CHECK(!acq.history()[0].cancellation);
    CHECK(!acq.history()[1].approved);
    CHECK(acq.history()[2].cancellation);

    CHECK(b.spent() == Money{});
    CHECK(b.usageFor(ResourceCategory::Book).units == 0);
    CHECK(b.usageFor(ResourceCategory::Book).titles == 0);
    CHECK(c.holdings("B1") == 0);
    CHECK(acq.totalSpent() == Money{});

    CHECK_THROWS(acq.cancelOrder(originalOrderNo), std::invalid_argument);
    CHECK_THROWS(acq.cancelOrder(2), std::invalid_argument);
    CHECK_THROWS(acq.cancelOrder(999), std::invalid_argument);

    CHECK(acq.canPurchase("B2", 1));
    acq.purchase("B2", 1);

    CHECK(b.usageFor(ResourceCategory::Book).titles == 1);
    CHECK(b.spent() == Money::of(100));
    CHECK(c.holdings("B2") == 1);
}

static void testDepartmentBudgets() {
    Catalog c;

    c.emplace<Book>(
        "B1", "Clean Code",
        std::vector<std::string>{"Author"},
        "ISBN1", "Publisher", 2020, Money::of(100)
    );

    c.emplace<Book>(
        "B2", "Physics Book",
        std::vector<std::string>{"Author"},
        "ISBN2", "Publisher", 2021, Money::of(100)
    );

    Budget defaultBudget(Money::of(5000));
    defaultBudget.setQuota(ResourceCategory::Book,
                          {10, Money::of(5000)});

    Budget csBudget(Money::of(300));
    csBudget.setQuota(ResourceCategory::Book,
                      {5, Money::of(200), 2});

    Budget physicsBudget(Money::of(500));
    physicsBudget.setQuota(ResourceCategory::Book,
                           {5, Money::of(400), 2});

    AcquisitionManager acq(c, defaultBudget);

    acq.registerDepartment("Computer Science", csBudget);
    acq.registerDepartment("Physics", physicsBudget);

    CHECK_THROWS(
        acq.registerDepartment("Physics", csBudget),
        std::invalid_argument
    );

    CHECK_THROWS(
        acq.registerDepartment("Biology", csBudget),
        std::invalid_argument
    );

    const int defaultOrderNo = acq.purchase("B1", 1).orderNo;

    CHECK(acq.history().back().department.empty());
    CHECK(defaultBudget.spent() == Money::of(100));

    CHECK(acq.canPurchase("B1", 2, "Computer Science"));

    const int csOrderNo =
        acq.purchase("B1", 2, "Computer Science").orderNo;

    CHECK(acq.history().back().department == "Computer Science");
    CHECK(csBudget.spent() == Money::of(200));
    CHECK(defaultBudget.spent() == Money::of(100));
    CHECK(physicsBudget.spent() == Money{});

    std::string why;

    CHECK(!acq.canPurchase("B2", 1, "Computer Science", &why));
    CHECK(why.find("spend quota") != std::string::npos);

    CHECK(acq.canPurchase("B2", 2, "Physics"));

    const int physicsOrderNo =
        acq.purchase("B2", 2, "Physics").orderNo;

    CHECK(acq.history().back().department == "Physics");
    CHECK(physicsBudget.spent() == Money::of(200));
    CHECK(csBudget.spent() == Money::of(200));

    CHECK(!acq.canPurchase("B1", 1, "Biology", &why));
    CHECK(why.find("department not found") != std::string::npos);

    auto results = acq.processBatch({
        {"B1", 1, "Physics"},
        {"B2", 1, "Biology"}
    });

    CHECK(results.size() == 2);
    CHECK(results[0].approved);
    CHECK(results[0].department == "Physics");
    CHECK(!results[1].approved);
    CHECK(results[1].department == "Biology");
    CHECK(results[1].reason.find("department not found") !=
          std::string::npos);

    CHECK(physicsBudget.spent() == Money::of(300));
    CHECK(csBudget.spent() == Money::of(200));
    CHECK(defaultBudget.spent() == Money::of(100));

    const auto& cancellation = acq.cancelOrder(csOrderNo);

    CHECK(cancellation.cancellation);
    CHECK(cancellation.relatedOrderNo == csOrderNo);
    CHECK(cancellation.department == "Computer Science");

    CHECK(csBudget.spent() == Money{});
    CHECK(csBudget.usageFor(ResourceCategory::Book).units == 0);
    CHECK(csBudget.usageFor(ResourceCategory::Book).titles == 0);

    CHECK(physicsBudget.spent() == Money::of(300));
    CHECK(defaultBudget.spent() == Money::of(100));
    CHECK(c.holdings("B1") == 2);

    CHECK(acq.history()[defaultOrderNo - 1].department.empty());
    CHECK(acq.history()[physicsOrderNo - 1].department == "Physics");
}

static void testAcquisition() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(500));
    b.setQuota(ResourceCategory::Book, {3, Money::of(1000)});
    AcquisitionManager acq(c, b);

    CHECK(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    CHECK(acq.canPurchase("B1", 3, &why) && why.empty());
    CHECK(!acq.canPurchase("B1", 4, &why) && !why.empty());
    CHECK(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);
    CHECK(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    CHECK(c.holdings("B1") == 2);

    CHECK_THROWS(acq.purchase("B1", 2), QuotaExceededError);
    CHECK_THROWS(acq.purchase("nope", 1), NotFoundError);
    CHECK(acq.history().size() == 1);  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    CHECK(res.size() == 5);
    CHECK(res[0].approved && res[0].cost == Money::of(150));
    CHECK(!res[1].approved);  // 1050 > remaining 150
    CHECK(res[2].approved);
    CHECK(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
    CHECK(!res[4].approved);
    CHECK(acq.totalSpent() == Money::of(450));
    CHECK(b.spent() == acq.totalSpent());
    CHECK(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    CHECK(acq.history().size() == 6);
}

int main() {
    testMoney();
    testResourcesAndCost();
    testJournal();
    testMagazine();
    testEBook();
    testAudioBookAndThesis();
    testHardcoverPricing();
    testBulkDiscounts();
    testTaxes();
    testCatalog();
    testBudget();
    testBudgetRollover();
    testBudgetWarnings();
    testTitleQuota();
    testCancelOrder();
    testDepartmentBudgets();
    testAcquisition();
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
