// CryptRelay_Test.cpp
// Unit testing for CryptRelay

#include <stdint.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <WS2tcpip.h>

#include "PortKnock.h"
#include "XBerkeleySockets.h"

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
	BankAccount account(1);
	EXPECT_EQ(0, account.balance);
}




TEST(PortKnockTest, PortIsNotInUse)
{
	// Making a class instance every time is not ideal, and should be avoided.
	XBerkeleySockets XBerkeleySocketsInstance;
	PortKnock PK(&XBerkeleySocketsInstance);

	EXPECT_EQ(0, PK.isLocalPortInUse("30000", "192.168.1.115"));
}


int32_t main(int argc, char * argv[])
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}