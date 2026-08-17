#if 0
// todo el contenido del archivo


#include "unity.h"
#include "led.h"

TEST_CASE("LedsInit returns success", "[board_support][led]")
{
    TEST_ASSERT_TRUE(LedsInit());
}

TEST_CASE("LedOn and LedOff return success for valid LEDs", "[board_support][led]")
{
    TEST_ASSERT_TRUE(LedOn(LED_1));
    TEST_ASSERT_TRUE(LedOn(LED_2));
    TEST_ASSERT_TRUE(LedOn(LED_3));

    TEST_ASSERT_TRUE(LedOff(LED_1));
    TEST_ASSERT_TRUE(LedOff(LED_2));
    TEST_ASSERT_TRUE(LedOff(LED_3));
}

TEST_CASE("LedToggle returns success for valid LEDs", "[board_support][led]")
{
    TEST_ASSERT_TRUE(LedToggle(LED_1));
    TEST_ASSERT_TRUE(LedToggle(LED_2));
    TEST_ASSERT_TRUE(LedToggle(LED_3));
}

TEST_CASE("LedsOffAll and LedsMask return success", "[board_support][led]")
{
    TEST_ASSERT_TRUE(LedsOffAll());
    TEST_ASSERT_TRUE(LedsMask(LED_1 | LED_3));
    TEST_ASSERT_TRUE(LedsMask(0));
}

TEST_CASE("Invalid LED values return failure", "[board_support][led]")
{
    TEST_ASSERT_FALSE(LedOn((led_t)0));
    TEST_ASSERT_FALSE(LedOff((led_t)0));
    TEST_ASSERT_FALSE(LedToggle((led_t)0));
}


#endif