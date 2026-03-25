// File: pimCmdFuse.cpp
// PIMeval Simulator - PIM API Fusion
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include "pimCmdFuse.h"
#include <cstdio>


//! @brief  Pim CMD: PIM API Fusion
PimStatus
pimCmdFuse::execute()
{
  if (m_debugCmds) {
    std::printf("PIM-Cmd: API Fusion\n");
  }

  // Functional simulation
  // TODO: skip original updateStats
  for (auto& api : m_prog.m_apis) {
    PimStatus status = api();
    if (status != PIM_OK) {
      return status;
    }
  }

  // Analyze API fusion opportunities
  return updateStats();
}

//! @brief  Pim CMD: PIM API Fusion - update stats
PimStatus
pimCmdFuse::updateStats() const
{
  // TODO: Parse m_prog and update stats
  return PIM_OK;
}

