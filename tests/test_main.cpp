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
    testEBook();
    testAudioBookAndThesis();
    testCatalog();
    testBudget();
    testAcquisition();
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
