#include <array>
#include <unity.h>

#include "core/FeatureRunner.h"
#include "drivers/IrTxDriver.h"
#include "drivers/IrRxDriver.h"
#include "drivers/Rf433TxDriver.h"
#include "drivers/Rf433RxDriver.h"
#include "features/ir/IrFeature.h"
#include "features/rf433/Rf433Feature.h"

void test_gpio_drivers_use_independent_ids_and_mutex_group() {
    TEST_ASSERT_NOT_EQUAL(PeripheralId::IrTx, PeripheralId::IrRx);
    TEST_ASSERT_NOT_EQUAL(PeripheralId::Rf433Tx, PeripheralId::Rf433Rx);
    TEST_ASSERT_EQUAL(2, IrTxDriver::mutexGroup());
    TEST_ASSERT_EQUAL(2, Rf433RxDriver::mutexGroup());

    const auto& irRoute = findRoute(BoardId::Esp32Cyd, PeripheralId::IrTx);
    IrTxDriver irTx;
    TEST_ASSERT_EQUAL(PeripheralStatus::NotFound, irTx.probe(irRoute, 10).status);
    irTx.setLoopbackConfirmed(true);
    TEST_ASSERT_EQUAL(PeripheralStatus::Available, irTx.probe(irRoute, 10).status);

    const auto& rfRoute = findRoute(BoardId::Esp32Cyd, PeripheralId::Rf433Rx);
    Rf433RxDriver rfRx;
    TEST_ASSERT_EQUAL(PeripheralStatus::NotFound, rfRx.probe(rfRoute, 10).status);
}

void test_ir_and_rf433_models_validate_and_round_trip() {
    IrFrame ir;
    ir.carrierHz = 38000;
    ir.pulsesUs = {9000, 4500, 560, 560};
    TEST_ASSERT_TRUE(IrCodec::valid(ir));
    const auto raw = IrCodec::encode(ir);
    TEST_ASSERT_EQUAL(ir.pulsesUs.size(), raw.size());

    Rf433Signal signal;
    signal.frequencyHz = 433920000;
    signal.code = 0x12345;
    signal.bits = 20;
    signal.protocol = 1;
    TEST_ASSERT_TRUE(Rf433Codec::valid(signal));
    TEST_ASSERT_FALSE(signal.transmitAuthorized);
}

void test_tx_and_rx_feature_factories_have_safe_lifecycle() {
    IFeature* ir = createIrFeature();
    IFeature* rf = createRf433Feature();
    FeatureContext context{};
    TEST_ASSERT_EQUAL(FeatureResultCode::Completed, ir->begin(context).code);
    TEST_ASSERT_EQUAL(FeatureResultCode::Completed, rf->begin(context).code);
    ir->requestStop(StopReason::UserRequest);
    rf->requestStop(StopReason::UserRequest);
    delete ir;
    delete rf;
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_gpio_drivers_use_independent_ids_and_mutex_group);
    RUN_TEST(test_ir_and_rf433_models_validate_and_round_trip);
    RUN_TEST(test_tx_and_rx_feature_factories_have_safe_lifecycle);
    return UNITY_END();
}
