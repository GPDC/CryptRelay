// CryptRelay_Test.cpp
// Unit testing for CryptRelay
// This is being written after CryptRelay has been created.
// It would be better to have written it during the creation of
// CryptRelay, but I didn't know about Unit Testing at the time.

#ifdef __linux__
// TODO
#endif//__linux__

#ifdef _WIN32
#include <stdint.h>
#include <WS2tcpip.h>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "PortKnock.h"
#include "XBerkeleySockets.h"
#endif// _WIN32

// The structure for a sample TEST, not related to CryptRelay
struct BankAccount
{
	int balance = 0;

	BankAccount()
	{

	}
	explicit BankAccount(const int balance)
		: balance(balance)
	{

	}
};

// A sample TEST, not related to CryptRelay
TEST(AccountTest, BankAccountStartsEmpty)
{
	BankAccount account(0);
	EXPECT_EQ(0, account.balance);
}

class PortKnockTest : public testing::Test
{
public:
	XBerkeleySockets * XBerkeleySocketsInstance = nullptr;
	PortKnock * PK = nullptr;
	PortKnockTest()
	{
		XBerkeleySocketsInstance = new XBerkeleySockets;
		PK = new PortKnock(XBerkeleySocketsInstance);
	}
	virtual ~PortKnockTest()
	{
		delete XBerkeleySocketsInstance;
		delete PK;
	}
};

TEST_F(PortKnockTest, PortIsNotInUse)
{
	EXPECT_EQ(0, PK->isLocalPortInUse("30000", "192.168.1.115"));
}

TEST_F(PortKnockTest, WHATISTHIS)
{
	PK->isPortOpen("192.168.1.115", "30000");
}

//TEST(PortKnockTest, PortIsNotInUse_REGTEST)
//{
//	// Making a class instance every time is not ideal, and should be avoided.
//	XBerkeleySockets XBerkeleySocketsInstance;
//	PortKnock PK(&XBerkeleySocketsInstance);
//
//	EXPECT_EQ(0, PK.isLocalPortInUse("30000", "192.168.1.115"));
//}


int32_t main(int argc, char * argv[])
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}

// TEST_F aka test fixture:
// If you find yourself writing two or more tests that operate on similar data, you can use a test fixture.
// It allows you to reuse the same configuration of objects for several different tests.

// DO NOT put underscores '_' in a TEST(TestCaseName, TestName) {};

// TEST_F is a text fixture class
// TEST is just the normal test. Visit the definition of TEST if you want more info.

// ASSERT_*   use this when it makes sense to immediately abort the current function
// EXPECT_*   otherwise use this when it makes sense to continue the function even if a test failed.

// The motto of TDD:
// 1. Decide what the code will do
// 2. Write a test that will pass if the code does that thing
// 3. Run the test, see it fail
// 4. Write the code
// 5. Run the test, see it pass
// You do this for every couple lines of code, every method, everything.
// Make 100% sure that the test can fail since it is possible to create a test that can't fail.
// If you write the test first, it makes it hard to make complicated code.
// Testable code is modular, decoupled, methods of limited scope.