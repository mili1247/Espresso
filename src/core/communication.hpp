/*
 * Copyright (C) 2010-2019 The ESPResSo project
 * Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010
 *   Max-Planck-Institute for Polymer Research, Theory Group
 *
 * This file is part of ESPResSo.
 *
 * ESPResSo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ESPResSo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _COMMUNICATION_HPP
#define _COMMUNICATION_HPP
/** \file
 *  This file contains the asynchronous MPI communication.
 *
 *  It is the header file for communication.cpp.
 *
 *  The asynchronous MPI communication is used during the script
 *  evaluation. Except for the master node that interprets the interface
 *  script, all other nodes wait in mpi_loop() for the master node to
 *  issue an action using mpi_call(). mpi_loop() immediately
 *  executes an MPI_Bcast and therefore waits for the master node to
 *  broadcast a command, which is done by mpi_call(). The request
 *  consists of a callback function and two arbitrary integers. If
 *  applicable, the first integer is the node number of the slave
 *  this request is dedicated to.
 *
 *  To add new actions (e.g. to implement new interface functionality), do the
 *  following:
 *  - write the @c mpi_* function that is executed on the master
 *  - write the @c mpi_*_slave function
 *  - in communication.cpp add your slave function to \ref CALLBACK_LIST or
 *    register it with one of the @c REGISTER_CALLBACK macros
 *
 *  After this, your procedure is free to do anything. However, it has
 *  to be in (MPI) sync with what your new @c mpi_*_slave does. This
 *  procedure is called immediately after the broadcast with the
 *  arbitrary integer as parameter. To this aim it has also to be added
 *  to \ref CALLBACK_LIST.
 */

#include "MpiCallbacks.hpp"

/* Includes needed by callbacks. */
#include "Particle.hpp"
#include "cuda_init.hpp"
#include "grid_based_algorithms/lb_constants.hpp"

#include <boost/mpi/communicator.hpp>
#include <vector>

/** The number of this node. */
extern int this_node;
/** The total number of nodes. */
extern int n_nodes;
/** The communicator */
extern boost::mpi::communicator comm_cart;
/** Statistics to calculate */
enum class GatherStats : int {
  lb_boundary_forces
};

/**
 * Default MPI tag used by callbacks.
 */
#ifndef SOME_TAG
#define SOME_TAG 42
#endif

namespace Communication {
/**
 * @brief Returns a reference to the global callback class instance.
 */
MpiCallbacks &mpiCallbacks();
} // namespace Communication

/**************************************************
 * for every procedure requesting a MPI negotiation,
 * a slave exists which processes this request on
 * the slave nodes. It is denoted by *_slave.
 **************************************************/

/** Initialize MPI. */
std::shared_ptr<boost::mpi::environment> mpi_init();

/** @brief Call a slave function.
 *  @tparam Args   Slave function argument types
 *  @tparam ArgRef Slave function argument types
 *  @param fp      Slave function
 *  @param args    Slave function arguments
 */
template <class... Args, class... ArgRef>
void mpi_call(void (*fp)(Args...), ArgRef &&... args) {
  Communication::mpiCallbacks().call(fp, std::forward<ArgRef>(args)...);
}

/** @brief Call a slave function.
 *  @tparam Args   Slave function argument types
 *  @tparam ArgRef Slave function argument types
 *  @param fp      Slave function
 *  @param args    Slave function arguments
 */
template <class... Args, class... ArgRef>
void mpi_call_all(void (*fp)(Args...), ArgRef &&... args) {
  Communication::mpiCallbacks().call_all(fp, std::forward<ArgRef>(args)...);
}

/** @brief Call a slave function.
 *  @tparam Tag    Any tag type defined in @ref Communication::Result
 *  @tparam R      Return type of the slave function
 *  @tparam Args   Slave function argument types
 *  @tparam ArgRef Slave function argument types
 *  @param tag     Reduction strategy
 *  @param fp      Slave function
 *  @param args    Slave function arguments
 */
template <class Tag, class R, class... Args, class... ArgRef>
auto mpi_call(Tag tag, R (*fp)(Args...), ArgRef &&... args) {
  return Communication::mpiCallbacks().call(tag, fp,
                                            std::forward<ArgRef>(args)...);
}

/** @brief Call a slave function.
 *  @tparam Tag    Any tag type defined in @ref Communication::Result
 *  @tparam TagArg Types of arguments to @p Tag
 *  @tparam R      Return type of the slave function
 *  @tparam Args   Slave function argument types
 *  @tparam ArgRef Slave function argument types
 *  @param tag     Reduction strategy
 *  @param tag_arg Arguments to the reduction strategy
 *  @param fp      Slave function
 *  @param args    Slave function arguments
 */
template <class Tag, class TagArg, class R, class... Args, class... ArgRef>
auto mpi_call(Tag tag, TagArg &&tag_arg, R (*fp)(Args...), ArgRef &&... args) {
  return Communication::mpiCallbacks().call(tag, std::forward<TagArg>(tag_arg),
                                            fp, std::forward<ArgRef>(args)...);
}

/** Process requests from master node. Slave nodes main loop. */
void mpi_loop();

/** Gather data for analysis.
 *  \param[in] job what to do:
 *      \arg for \ref GatherStats::lb_boundary_forces, use
 *           \ref lb_collect_boundary_forces.
 *  \param[out] result where to store values gathered by
 *      \ref GatherStats::lb_boundary_forces
 */
void mpi_gather_stats(GatherStats job, double *result = nullptr);

namespace Communication {
/**
 * @brief Init globals for communication.
 *
 * and calls @ref on_program_start. Keeps a copy of
 * the pointer to the mpi environment to keep it alive
 * while the program is loaded.
 *
 * @param mpi_env MPI environment that should be used
 */
void init(std::shared_ptr<boost::mpi::environment> mpi_env);
} // namespace Communication
#endif
