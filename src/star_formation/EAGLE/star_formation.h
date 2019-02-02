/*******************************************************************************
 * This file is part of SWIFT.
 * Copyright (c) 2018 Folkert Nobels (nobels@strw.leidenuniv.nl)
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
 *******************************************************************************/
#ifndef SWIFT_EAGLE_STAR_FORMATION_H
#define SWIFT_EAGLE_STAR_FORMATION_H

/* Local includes */
#include "adiabatic_index.h"
#include "cooling.h"
#include "cosmology.h"
#include "engine.h"
#include "equation_of_state.h"
#include "hydro.h"
#include "parser.h"
#include "part.h"
#include "physical_constants.h"
#include "stars.h"
#include "units.h"

/**
 * @file src/star_formation/EAGLE/star_formation.h
 * @brief Star formation model used in the EAGLE model
 */

/**
 * @brief Properties of the EAGLE star formation model.
 */
struct star_formation {

  /*! Normalization of the KS star formation law (internal units) */
  float KS_normalization;

  /*! Normalization of the KS star formation law (Msun / kpc^2 / yr) */
  float KS_normalization_MSUNpYRpKPC2;

  /*! Slope of the KS law */
  float KS_power_law;

  /*! Slope of the high density KS law */
  float KS_high_den_power_law;

  /*! KS law High density threshold (internal units) */
  float KS_high_den_thresh;

  /*! KS high density normalization (internal units) */
  float KS_high_den_normalization;

  /*! KS high density normalization (H atoms per cm^3)  */
  float KS_high_den_thresh_HpCM3;

  /*! Critical overdensity */
  float min_over_den;

  /*! Dalla Vecchia & Schaye temperature criteria */
  float temperature_margin_threshold_dex;

  /*! gas fraction */
  float fgas;

  /*! Star formation law slope */
  float SF_power_law;

  /*! star formation normalization (internal units) */
  float SF_normalization;

  /*! star formation high density slope */
  float SF_high_den_power_law;

  /*! Star formation high density normalization (internal units) */
  float SF_high_den_normalization;

  /*! Density threshold to form stars (internal units) */
  float density_threshold;

  /*! Density threshold to form stars in user units */
  float density_threshold_HpCM3;

  /*! Maximum density threshold to form stars (internal units) */
  float density_threshold_max;

  /*! Maximum density threshold to form stars (H atoms per cm^3) */
  float density_threshold_max_HpCM3;

  /*! Reference metallicity for metal-dependant threshold */
  float Z0;

  /*! Inverse of reference metallicity */
  float Z0_inv;

  /*! critical density Metallicity power law (internal units) */
  float n_Z0;

  /*! Polytropic index */
  float polytropic_index;

  /*! EOS pressure norm (internal units) */
  float EOS_pressure_norm;

  /*! EOS Temperature norm (internal units)  */
  float EOS_temperature_norm;

  /*! EOS density norm (internal units) */
  float EOS_density_norm;

  /*! EOS density norm (H atoms per cm^3) */
  float EOS_density_norm_HpCM3;

  /*! Max physical density (H atoms per cm^3)*/
  float max_gas_density_HpCM3;

  /*! Max physical density (internal units) */
  float max_gas_density;
};

/**
 * @brief Calculate if the gas has the potential of becoming
 * a star.
 *
 * @param starform the star formation law properties to use.
 * @param p the gas particles.
 * @param xp the additional properties of the gas particles.
 * @param phys_const the physical constants in internal units.
 * @param cosmo the cosmological parameters and properties.
 * @param hydro_props The properties of the hydro scheme.
 * @param us The internal system of units.
 * @param cooling The cooling data struct.
 *
 */
INLINE static int star_formation_potential_to_become_star(
    const struct star_formation* starform, const struct part* restrict p,
    const struct xpart* restrict xp, const struct phys_const* const phys_const,
    const struct cosmology* cosmo,
    const struct hydro_props* restrict hydro_props,
    const struct unit_system* restrict us,
    const struct cooling_function_data* restrict cooling) {

  /* Read the critical overdensity factor and the critical density of
   * the universe to determine the critical density to form stars*/
  const double rho_crit_times_min_over_den =
      cosmo->critical_density * starform->min_over_den;
  const double particle_density = hydro_get_physical_density(p, cosmo);

  /* Deside whether we should form stars or not,
   * first we deterime if we have the correct over density
   * if that is true we calculate if either the maximum density
   * threshold is reached or if the metallicity dependent
   * threshold is reached, after this we calculate if the
   * temperature is appropriate */
  if (particle_density < rho_crit_times_min_over_den) return 0;

  /* In this case there are actually multiple possibilities
   * because we also need to check if the physical density exceeded
   * the appropriate limit */

  const double Z = p->chemistry_data.smoothed_metal_mass_fraction_total;
  double density_threshold_metal_dep;
  if (Z > 0) {
    density_threshold_metal_dep =
        starform->density_threshold * pow(Z * starform->Z0_inv, starform->n_Z0);
  } else {
    density_threshold_metal_dep = starform->density_threshold_max;
  }

  /* Calculate the maximum between both and convert to mass density instead of
   * number density*/
  const double density_threshold_current =
      min(density_threshold_metal_dep, starform->density_threshold_max) *
      phys_const->const_proton_mass;

  /* Check if it exceeded the maximum density */
  if (particle_density * p->chemistry_data.smoothed_metal_mass_fraction[0] <
      density_threshold_current)
    return 0;

  /* Calculate the temperature */
  const double temperature = cooling_get_temperature(phys_const, hydro_props,
                                                     us, cosmo, cooling, p, xp);

  const double temperature_eos =
      starform->EOS_pressure_norm / phys_const->const_boltzmann_k *
      pow(particle_density * p->chemistry_data.smoothed_metal_mass_fraction[0],
          starform->polytropic_index - 1.f) *
      pow(starform->EOS_density_norm, starform->polytropic_index);

  /* Check the last criteria, if the temperature is satisfied */
  return (log10(temperature) <
          log10(temperature_eos) + starform->temperature_margin_threshold_dex);
}

/**
 * @brief Calculates if the gas particle gets converted
 *
 * @param e the #engine
 * @param starform the star formation law properties to use.
 * @param p the gas particles.
 * @param xp the additional properties of the gas particles.
 * @param phys_const the physical constants in internal units.
 * @param cosmo the cosmological parameters and properties.
 * @param hydro_props The properties of the hydro scheme.
 * @param us The internal system of units.
 * @param cooling The cooling data struct.
 * @param dt_star The time-step of this particle.
 * @param with_cosmology Are we running with cosmology on?
 */
INLINE static int star_formation_convert_to_star(
    const struct engine* e, const struct star_formation* starform,
    const struct part* restrict p, struct xpart* restrict xp,
    const struct phys_const* const phys_const, const struct cosmology* cosmo,
    const struct hydro_props* restrict hydro_props,
    const struct unit_system* restrict us,
    const struct cooling_function_data* restrict cooling, const double dt_star,
    const int with_cosmology) {

  if (dt_star == 0.f) return 0;

  if (star_formation_potential_to_become_star(
          starform, p, xp, phys_const, cosmo, hydro_props, us, cooling)) {
    /* Get the pressure */

    const double pressure =
        starform->EOS_pressure_norm *
        pow(hydro_get_physical_density(p, cosmo) *
                p->chemistry_data.smoothed_metal_mass_fraction[0] /
                starform->EOS_density_norm / phys_const->const_proton_mass,
            starform->polytropic_index);

    double SFRpergasmass;
    if (hydro_get_physical_density(p, cosmo) <
        starform->KS_high_den_thresh * phys_const->const_proton_mass) {
      /* Calculate the star formation rate */
      SFRpergasmass =
          starform->SF_normalization * pow(pressure, starform->SF_power_law);
    } else if (hydro_get_physical_density(p, cosmo) >
               starform->max_gas_density * phys_const->const_proton_mass) {
      /* We give the star formation tracers values of the mass and -1 for sSFR
       * to be able to trace them, we don't want random variables there*/
      xp->sf_data.SFR = p->mass;
      xp->sf_data.sSFR = -1.f;
      return 1.f;
    } else {
      SFRpergasmass = starform->SF_high_den_normalization *
                      pow(pressure, starform->SF_high_den_power_law);
    }

    /* Store the SFR */
    xp->sf_data.SFR = SFRpergasmass * p->mass;
    xp->sf_data.sSFR = SFRpergasmass;
    /* Calculate the propability of forming a star */
    const double prop = SFRpergasmass * dt_star;

    /* Calculate the seed */
    unsigned int seed = (p->id + e->ti_current) % 8191;

    /* Generate a random number between 0 and 1. */
    const double randomnumber =
        rand_r(&seed);  // MATTHIEU: * starform->inv_RAND_MAX;

    /* Calculate if we form a star */
    return (prop > randomnumber);
  }

  /* Check if it is the first time steps after star formation */
  if (xp->sf_data.SFR > 0.f) {
    if (with_cosmology) {
      xp->sf_data.SFR = -cosmo->a;
      xp->sf_data.sSFR = 0.f;
    } else {
      xp->sf_data.SFR = -e->time;
      xp->sf_data.sSFR = 0.f;
    }
  }

  return 0;
}

/**
 * @brief Copies the properties of the gas particle over to the
 * star particle
 *
 * @param e The #engine
 * @param p the gas particles.
 * @param xp the additional properties of the gas particles.
 * @param sp the new created star particle with its properties.
 * @param starform the star formation law properties to use.
 * @param phys_const the physical constants in internal units.
 * @param cosmo the cosmological parameters and properties.
 * @param with_cosmology if we run with cosmology.
 */
INLINE static void star_formation_copy_properties(
    const struct engine* e, const struct part* p, const struct xpart* xp,
    struct spart* sp, const struct star_formation* starform,
    const struct phys_const* const phys_const, const struct cosmology* cosmo,
    const int with_cosmology) {

  /* Store the current mass */
  sp->mass = p->mass;

  /* Store the current mass as the initial mass */
  sp->mass_init = p->mass;

  /* Store either the birth_scale_factor or birth_time depending  */
  if (with_cosmology) {
    sp->birth_scale_factor = cosmo->a;
  } else {
    sp->birth_time = e->time;
  }

  /* Store the chemistry struct in the star particle */
  sp->chemistry_data = p->chemistry_data;

  /* Store the tracers data */
  sp->tracers_data = xp->tracers_data;

  /* Store the birth density in the star particle */
  sp->birth_density = hydro_get_physical_density(p, cosmo);

  sp->new_star_flag = 1;

  message("A star has been formed!");
}

/**
 * @brief initialization of the star formation law
 *
 * @param parameter_file The parsed parameter file
 * @param phys_const Physical constants in internal units
 * @param us The current internal system of units.
 * @param hydro_props The propertis of the hydro model.
 * @param starform the star formation law properties to initialize
 */
INLINE static void starformation_init_backend(
    struct swift_params* parameter_file, const struct phys_const* phys_const,
    const struct unit_system* us, const struct hydro_props* hydro_props,
    struct star_formation* starform) {

  /* Get the Gravitational constant */
  const float G_newton = phys_const->const_newton_G;

  /* Initial Hydrogen abundance (mass fraction) */
  const float X_H = hydro_props->hydrogen_mass_fraction;

  /* Mean molecular weight assuming neutral gas */
  const float mean_molecular_weight = hydro_props->mu_neutral;

  /* Get the surface density unit Msun / pc^2 in internal units */
  const float Msun_per_pc2 =
      phys_const->const_solar_mass /
      (phys_const->const_parsec * phys_const->const_parsec);

  /* Get the SF surface density unit Msun / pc^2 / yr in internal units */
  const float Msun_per_pc2_per_year = Msun_per_pc2 / phys_const->const_year;

  /* Conversion of number density from cgs */
  const float number_density_from_cgs =
      1. / units_cgs_conversion_factor(us, UNIT_CONV_NUMBER_DENSITY);

  /* Quantities that have to do with the Normal Kennicutt-
   * Schmidt law will be read in this part of the code*/

  /* Load the equation of state for this model */
  starform->polytropic_index = parser_get_param_float(
      parameter_file, "EAGLEStarFormation:EOS_gamma_effective");
  starform->EOS_temperature_norm = parser_get_param_float(
      parameter_file, "EAGLEStarFormation:EOS_temperature_norm_K");
  starform->EOS_density_norm_HpCM3 = parser_get_param_float(
      parameter_file, "EAGLEStarFormation:EOS_density_threshold_H_p_cm3");
  starform->EOS_density_norm =
      starform->EOS_density_norm_HpCM3 * number_density_from_cgs;

  /* Calculate the EOS pressure normalization */
  starform->EOS_pressure_norm =
      starform->EOS_density_norm * starform->EOS_temperature_norm *
      phys_const->const_boltzmann_k / mean_molecular_weight / X_H;

  /* Read the critical density contrast from the parameter file*/
  starform->min_over_den = parser_get_param_float(
      parameter_file, "EAGLEStarFormation:KS_min_over_density");

  /* Read the critical temperature from the parameter file */
  starform->temperature_margin_threshold_dex = parser_get_param_float(
      parameter_file, "EAGLEStarFormation:temperature_margin_threshold_dex");

  /* Read the gas fraction from the file */
  starform->fgas = parser_get_opt_param_float(
      parameter_file, "EAGLEStarFormation:gas_fraction", 1.f);

  /* Read the Kennicutt-Schmidt power law exponent */
  starform->KS_power_law =
      parser_get_param_float(parameter_file, "EAGLEStarFormation:KS_exponent");

  /* Calculate the power law of the corresponding star formation Schmidt law */
  starform->SF_power_law = (starform->KS_power_law - 1.f) / 2.f;

  /* Read the normalization of the KS law in KS law units */
  starform->KS_normalization_MSUNpYRpKPC2 = parser_get_param_float(
      parameter_file, "EAGLEStarFormation:KS_normalisation");

  /* Convert to internal units */
  starform->KS_normalization =
      starform->KS_normalization_MSUNpYRpKPC2 * Msun_per_pc2_per_year;

  /* Calculate the starformation pre-factor (eq. 12 of Schaye & Dalla Vecchia
   * 2008) */
  starform->SF_normalization =
      starform->KS_normalization * pow(Msun_per_pc2, -starform->KS_power_law) *
      pow(hydro_gamma * starform->fgas / G_newton, starform->SF_power_law);

  /* Read the high density Kennicutt-Schmidt power law exponent */
  starform->KS_high_den_power_law = parser_get_param_float(
      parameter_file, "EAGLEStarFormation:KS_high_density_exponent");

  /* Calculate the SF high density power law */
  starform->SF_high_den_power_law =
      (starform->KS_high_den_power_law - 1.f) / 2.f;

  /* Read the high density criteria for the KS law in number density per cm^3 */
  starform->KS_high_den_thresh_HpCM3 = parser_get_param_float(
      parameter_file, "EAGLEStarFormation:KS_high_density_threshold_H_p_cm3");

  /* Transform the KS high density criteria to simulation units */
  starform->KS_high_den_thresh =
      starform->KS_high_den_thresh_HpCM3 * number_density_from_cgs;

  /* Pressure at the high-density threshold */
  const float EOS_high_den_pressure =
      starform->EOS_pressure_norm *
      pow(starform->KS_high_den_thresh / starform->EOS_density_norm,
          starform->polytropic_index);

  /* Calculate the KS high density normalization
   * We want the SF law to be continous so the normalisation of the second
   * power-law is the value of the first power-law at the high-density threshold
   */
  starform->KS_high_den_normalization =
      starform->KS_normalization *
      pow(Msun_per_pc2,
          starform->KS_high_den_power_law - starform->KS_power_law) *
      pow(hydro_gamma * EOS_high_den_pressure * starform->fgas / G_newton,
          (starform->KS_power_law - starform->KS_high_den_power_law) * 0.5f);

  /* Calculate the SF high density normalization */
  starform->SF_high_den_normalization =
      starform->KS_high_den_normalization *
      pow(Msun_per_pc2, -starform->KS_high_den_power_law) *
      pow(hydro_gamma * starform->fgas / G_newton,
          starform->SF_high_den_power_law);

  /* Get the maximum physical density for SF */
  starform->max_gas_density_HpCM3 = parser_get_opt_param_float(
      parameter_file, "EAGLEStarFormation:KS_max_density_threshold_H_p_cm3",
      FLT_MAX);

  /* Convert the maximum physical density to internal units */
  starform->max_gas_density =
      starform->max_gas_density_HpCM3 * number_density_from_cgs;

  starform->temperature_margin_threshold_dex = parser_get_opt_param_float(
      parameter_file, "EAGLEStarFormation:KS_temperature_margin", FLT_MAX);

  /* Read the normalization of the metallicity dependent critical
   * density*/
  starform->density_threshold_HpCM3 = parser_get_param_float(
      parameter_file, "EAGLEStarFormation:threshold_norm_H_p_cm3");

  /* Convert to internal units */
  starform->density_threshold =
      starform->density_threshold_HpCM3 * number_density_from_cgs;

  /* Read the scale metallicity Z0 */
  starform->Z0 = parser_get_param_float(parameter_file,
                                         "EAGLEStarFormation:threshold_Z0");
  starform->Z0_inv = 1.f / starform->Z0;

  /* Read the power law of the critical density scaling */
  starform->n_Z0 = parser_get_param_float(
      parameter_file, "EAGLEStarFormation:threshold_slope");

  /* Read the maximum allowed density for star formation */
  starform->density_threshold_max_HpCM3 = parser_get_param_float(
      parameter_file, "EAGLEStarFormation:threshold_max_density_H_p_cm3");

  /* Convert to internal units */
  starform->density_threshold_max =
      starform->density_threshold_max_HpCM3 * number_density_from_cgs;
}

/**
 * @brief Prints the used parameters of the star formation law
 *
 * @param starform the star formation law properties.
 * */
INLINE static void starformation_print_backend(
    const struct star_formation* starform) {

  message("Star formation law is EAGLE (Schaye & Dalla Vecchia 2008)");
  message(
      "With properties: normalization = %e Msun/kpc^2/yr, slope of the"
      "Kennicutt-Schmidt law = %e and gas fraction = %e ",
      starform->KS_normalization_MSUNpYRpKPC2, starform->KS_power_law,
      starform->fgas);
  message("At densities of %e H/cm^3 the slope changes to %e.",
          starform->KS_high_den_thresh_HpCM3, starform->KS_high_den_power_law);
  message(
      "The effective equation of state is given by: polytropic "
      "index = %e , normalization density = %e #/cm^3 and normalization "
      "temperature = %e K",
      starform->polytropic_index, starform->EOS_density_norm_HpCM3,
      starform->EOS_temperature_norm);
  message("Density threshold follows Schaye (2004)");
  message(
      "the normalization of the density threshold is given by"
      " %e #/cm^3, with metallicity slope of %e, and metallicity normalization"
      " of %e, the maximum density threshold is given by %e #/cm^3",
      starform->density_threshold_HpCM3, starform->n_Z0, starform->Z0,
      starform->density_threshold_max_HpCM3);
  message("Temperature threshold is given by Dalla Vecchia and Schaye (2012)");
  message("The temperature threshold offset from the EOS is given by: %e dex",
          starform->temperature_margin_threshold_dex);
  message("Running with a maximum gas density given by: %e #/cm^3",
          starform->max_gas_density_HpCM3);
}

#endif /* SWIFT_EAGLE_STAR_FORMATION_H */