#include "udf.h"

/*
 * ANSYS UDF: Inlet Boundary Mass Flow Rate Calculation
 * 
 * For Transient Analysis: Calculates inlet mass flow rate based on fluid mass reduction
 */

/* Global Variables */
static real initial_mass = 0.0;             /* Initial fluid mass (kg) */
static real initial_supply_pressure = 0.0; /* Initial supply pressure (Pa) */
static char inlet_boundary_name[256] = "";  /* Inlet boundary name */

static real current_total_mass = 0.0;       /* Current total fluid mass (kg) */

/*
 * Function: Calculate_Inlet_Mass_Flow_Rate
 * Purpose: Calculate inlet mass flow rate proportional to fluid mass (transient analysis)
 * 
 * Return: Calculated inlet mass flow rate (kg/s)
 * 
 * Formula:
 * new_mass_flow_rate = base_mass_flow_rate * (current_total_mass / initial_mass)
 */
real Calculate_Inlet_Mass_Flow_Rate(
    real total_mass,              /* Current total fluid mass (kg) */
    real base_mass_flow_rate)     /* Base inlet mass flow rate (kg/s) */
{
    real new_mass_flow_rate;
    real mass_ratio;
    
    if (initial_mass > 0.0)
    {
        mass_ratio = total_mass / initial_mass;
        new_mass_flow_rate = base_mass_flow_rate * mass_ratio;
        
        if (new_mass_flow_rate < 0.0)
            new_mass_flow_rate = 0.0;
    }
    else
    {
        new_mass_flow_rate = base_mass_flow_rate;
    }
    
    return new_mass_flow_rate;
}

/*
 * Function: DEFINE_PROFILE (inlet_mass_flow_rate_profile)
 * Purpose: Define mass flow rate profile at inlet boundary
 */
DEFINE_PROFILE(inlet_mass_flow_rate_profile, thread, position)
{
    real mass_flow_rate;
    face_t f;
    
    /* *** [USER INPUT] Base inlet mass flow rate (kg/s) - Modify this value *** */
    real base_mass_flow_rate = 1.0;  
    
    real current_mass = current_total_mass;
    
    begin_f_loop(f, thread)
    {
        mass_flow_rate = Calculate_Inlet_Mass_Flow_Rate(current_mass, base_mass_flow_rate);
        F_PROFILE(f, thread, position) = mass_flow_rate;
    }
    end_f_loop(f, thread)
}

/*
 * Function: Set_Total_Mass
 * Purpose: Update current total fluid mass for each time step
 */
void Set_Total_Mass(real mass)
{
    current_total_mass = mass;
}

/*
 * Function: Get_Inlet_Mass_Flow_Rate
 * Purpose: Query calculated inlet mass flow rate
 */
real Get_Inlet_Mass_Flow_Rate(real base_mass_flow_rate)
{
    return Calculate_Inlet_Mass_Flow_Rate(current_total_mass, base_mass_flow_rate);
}

/*
 * Function: Reset_Total_Mass
 * Purpose: Initialize before analysis
 */
void Reset_Total_Mass(void)
{
    current_total_mass = 0.0;
}

/*
 * Function: Set_Initial_Parameters
 * Purpose: Set initial parameters
 * 
 * *** [USER INPUT] Call this function before analysis with actual values ***
 * Example: Set_Initial_Parameters(100.0, 101325.0, "inlet");
 *          - First argument: Initial fluid mass (kg)
 *          - Second argument: Initial supply pressure (Pa)
 *          - Third argument: Inlet boundary name (string)
 */
void Set_Initial_Parameters(real mass, real supply_pressure, const char *boundary_name)
{
    initial_mass = mass;
    initial_supply_pressure = supply_pressure;
    
    if (boundary_name != NULL)
    {
        strncpy(inlet_boundary_name, boundary_name, sizeof(inlet_boundary_name) - 1);
        inlet_boundary_name[sizeof(inlet_boundary_name) - 1] = '\0';
    }
}

/*
 * Function: Get_Initial_Mass
 * Purpose: Query initial fluid mass
 * 
 * Return: Initial fluid mass (kg)
 */
real Get_Initial_Mass(void)
{
    return initial_mass;
}

/*
 * Function: Get_Initial_Supply_Pressure
 * Purpose: Query initial supply pressure
 * 
 * Return: Initial supply pressure (Pa)
 */
real Get_Initial_Supply_Pressure(void)
{
    return initial_supply_pressure;
}

/*
 * Function: Get_Inlet_Boundary_Name
 * Purpose: Query inlet boundary name
 * 
 * Return: Inlet boundary name (string pointer)
 */
const char* Get_Inlet_Boundary_Name(void)
{
    return inlet_boundary_name;
}
