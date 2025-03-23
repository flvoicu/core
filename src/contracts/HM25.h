using namespace QPI;

struct HM252
{
};

struct HM25 : public ContractBase
{
public:
    // Procedimientos y funciones originales
    struct Echo_input{};
    struct Echo_output{};

    struct Burn_input{};
    struct Burn_output{};

    struct GetStats_input {};
    struct GetStats_output
    {
        uint64 numberOfEchoCalls;
        uint64 numberOfBurnCalls;
    };

    // Nuevos structs para escrow
    struct DepositEscrow_input{};
    struct DepositEscrow_output{};

    struct ConfirmByBuyer_input{};
    struct ConfirmByBuyer_output{};

    struct ConfirmBySeller_input{};
    struct ConfirmBySeller_output{};

    struct CancelTransaction_input{};
    struct CancelTransaction_output{};

private:
    uint64 numberOfEchoCalls;
    uint64 numberOfBurnCalls;

    // Variables de estado para la funcionalidad de escrow
    // escrowStatus: 0 = Aguardando depósito, 1 = Depósito realizado, 2 = Fondos liberados, 3 = Transacción cancelada.
    uint64 escrowAmount;
    uint8 escrowStatus;
    QPI::id buyerAddress;
    QPI::id sellerAddress;
    bool buyerConfirmed;
    bool sellerConfirmed;

    /**
    * Procedimiento original: Envía de vuelta la cantidad de invocación.
    */
    PUBLIC_PROCEDURE(Echo)
        state.numberOfEchoCalls++;
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
    _

    /**
    * Procedimiento original: Quema la cantidad de invocación.
    */
    PUBLIC_PROCEDURE(Burn)
        state.numberOfBurnCalls++;
        if (qpi.invocationReward() > 0)
        {
            qpi.burn(qpi.invocationReward());
        }
    _

    /**
    * Función original: Retorna estadísticas.
    */
    PUBLIC_FUNCTION(GetStats)
        output.numberOfBurnCalls = state.numberOfBurnCalls;
        output.numberOfEchoCalls = state.numberOfEchoCalls;
    _

    // --- Funciones de Escrow ---

    /**
    * Procedimiento para que el comprador deposite fondos en escrow.
    * Se utiliza la recompensa de invocación como monto a depositar.
    * Sólo es válido si aún no se ha realizado un depósito (estado 0).
    */
    PUBLIC_PROCEDURE(DepositEscrow)
        if (state.escrowStatus != 0) {
            return;
        }
        if (qpi.invocationReward() == 0) {
            return;
        }
        state.buyerAddress = qpi.invocator();
        state.escrowAmount = qpi.invocationReward();
        state.escrowStatus = 1; // Depósito realizado
        state.buyerConfirmed = false;
        state.sellerConfirmed = false;
    _

    /**
    * Procedimiento para que el comprador confirme la transacción.
    * Se marca la confirmación del comprador y, si el vendedor ya confirmó, se libera el monto al vendedor.
    */
    PUBLIC_PROCEDURE(ConfirmByBuyer)
        if (state.escrowStatus != 1) {
            return;
        }
        if (qpi.invocator() != state.buyerAddress) {
            return;
        }
        state.buyerConfirmed = true;
        // Si ambos han confirmado, se libera el monto al vendedor.
        if (state.buyerConfirmed && state.sellerConfirmed) {
            qpi.transfer(state.sellerAddress, state.escrowAmount);
            state.escrowAmount = 0;
            state.escrowStatus = 2; // Fondos liberados
        }
    _

    /**
    * Procedimiento para que el vendedor confirme la transacción.
    * Se marca la confirmación del vendedor y, si el comprador ya confirmó, se libera el monto al vendedor.
    */
    PUBLIC_PROCEDURE(ConfirmBySeller)
        if (state.escrowStatus != 1) {
            return;
        }
        if (qpi.invocator() != state.sellerAddress) {
            return;
        }
        state.sellerConfirmed = true;
        // Si ambos han confirmado, se libera el monto al vendedor.
        if (state.buyerConfirmed && state.sellerConfirmed) {
            qpi.transfer(state.sellerAddress, state.escrowAmount);
            state.escrowAmount = 0;
            state.escrowStatus = 2; // Fondos liberados
        }
    _

    /**
    * Procedimiento para cancelar la transacción.
    * Puede ser invocado por el comprador o el vendedor si la transacción aún no se ha completado.
    * Se reembolsa al comprador el monto depositado.
    */
    PUBLIC_PROCEDURE(CancelTransaction)
        if (state.escrowStatus != 1) {
            return;
        }
        // Sólo el comprador o el vendedor pueden cancelar.
        if (qpi.invocator() != state.buyerAddress && qpi.invocator() != state.sellerAddress) {
            return;
        }
        qpi.transfer(state.buyerAddress, state.escrowAmount);
        state.escrowAmount = 0;
        state.escrowStatus = 3; // Transacción cancelada
    _

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES

        REGISTER_USER_PROCEDURE(Echo, 1);
        REGISTER_USER_PROCEDURE(Burn, 2);
        REGISTER_USER_PROCEDURE(DepositEscrow, 3);
        REGISTER_USER_PROCEDURE(ConfirmByBuyer, 4);
        REGISTER_USER_PROCEDURE(ConfirmBySeller, 5);
        REGISTER_USER_PROCEDURE(CancelTransaction, 6);

        REGISTER_USER_FUNCTION(GetStats, 1);
    _

    INITIALIZE
        state.numberOfEchoCalls = 0;
        state.numberOfBurnCalls = 0;
        state.escrowAmount = 0;
        state.escrowStatus = 0; // Aguardando depósito
        // Se fija la dirección del vendedor; en producción se debe establecer dinámicamente o mediante parámetros.
        state.sellerAddress = QPI::id((const unsigned char*)"seller_address");
        state.buyerConfirmed = false;
        state.sellerConfirmed = false;
    _
};