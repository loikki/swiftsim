/*******************************************************************************
 * This file is part of SWIFT.
 * Copyright (c) 2016 Matthieu Schaller (matthieu.schaller@durham.ac.uk)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/
#ifndef SWIFT_DEFAULT_STAR_PART_H
#define SWIFT_DEFAULT_STAR_PART_H

/* Some standard headers. */
#include <stdlib.h>

/* Read chemistry */
#include "chemistry_struct.h" 

/**
 * @brief Particle fields for the star particles.
 *
 * All quantities related to gravity are stored in the associate #gpart.
 */
struct spart {

  /*! Particle ID. */
  long long id;

  /*! Pointer to corresponding gravity part. */
  struct gpart* gpart;

  /*! Particle position. */
  double x[3];

  /* Offset between current position and position at last tree rebuild. */
  float x_diff[3];

  /* Offset between current position and position at last tree rebuild. */
  float x_diff_sort[3];

  /*! Particle velocity. */
  float v[3];

  /*! Star mass */
  float mass;

  /*! Initial star mass */
  float mass_init;

  /* Particle cutoff radius. */
  float h;

  /*! Particle time bin */
  timebin_t time_bin;

  struct {
    /* Number of neighbours. */
    float wcount;

    /* Number of neighbours spatial derivative. */
    float wcount_dh;

  } density;

  struct {
    /* Mass of ejecta */
    float mass;

    /* Mass fractions of ejecta */
    struct chemistry_part_data chemistry_data;
  
    float ejecta_specific_thermal_energy;

    /* Number of type 1a SNe per unit mass */
    float num_SNIa;

    /* total mass of neighbouring gas particles (TODO: CHECK IF THIS IS DIFFERENT THAN ngb_mass DECLARED BELOW) */
    float ngb_mass;

  } to_distribute;

  /* kernel normalisation factor (equivalent to metalweight_norm in eagle_enrich.c:811, IMPROVE COMMENT) */
  float omega_normalisation_inv;

  /* total mass of neighbouring gas particles */
  float ngb_mass;

#ifdef SWIFT_DEBUG_CHECKS

  /* Time of the last drift */
  integertime_t ti_drift;

  /* Time of the last kick */
  integertime_t ti_kick;

#endif

#ifdef DEBUG_INTERACTIONS_STARS
  /*! List of interacting particles in the density SELF and PAIR */
  long long ids_ngbs_density[MAX_NUM_OF_NEIGHBOURS_STARS];

  /*! Number of interactions in the density SELF and PAIR */
  int num_ngb_density;
#endif

} SWIFT_STRUCT_ALIGN;

/**
 * @brief Contains all the constants and parameters of the stars scheme
 */
struct stars_props {

  /*! Resolution parameter */
  float eta_neighbours;

  /*! Target weightd number of neighbours (for info only)*/
  float target_neighbours;

  /*! Smoothing length tolerance */
  float h_tolerance;

  /*! Tolerance on neighbour number  (for info only)*/
  float delta_neighbours;

  /*! Maximal smoothing length */
  float h_max;

  /*! Maximal number of iterations to converge h */
  int max_smoothing_iterations;

  /*! Maximal change of h over one time-step */
  float log_max_h_change;

  int continuous_heating;

  float SNIa_energy_fraction;
  float deltaT_desired;
  float temp_to_u_factor;
  float total_energy_SNe;

  // Conversion factors copied from EAGLE. CHANGE NAME TO BE MORE DESCRIPTIVE
  float units_factor1, units_factor2;

  float feedback_timescale;

  // CHANGE THIS TO BE CONSISTENT WITH RAND MAX USED IN STAR FORMATION
  double inv_rand_max;
  
  // ALEXEI: FOR DEBUGGING
  float const_solar_mass;


};

#endif /* SWIFT_DEFAULT_STAR_PART_H */