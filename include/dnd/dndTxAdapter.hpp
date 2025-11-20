#pragma once

#include "core/transaction.hpp"
#include "dnd/dndTx.hpp"

Transaction wrapDndTxIntoTransaction(const dnd::DndEventTx& evt);
dnd::DndEventTx extractDndEventTx(const Transaction& tx);

