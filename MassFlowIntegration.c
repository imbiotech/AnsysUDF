#include "udf.h"
#include "sg_mphase.h"

/* Define global variables for initial mass and volume of the container */
/* Modify these values to match actual conditions */
real V_container = 10;      /* Container volume (m^3) */
real m_current;              /* Current mass in container (kg) - declared as global variable */
real T_container = 25+273.15;    /* Container temperature (K) - isothermal assumption */
real R_gas = 8.314*10^-3/88.15;          /* Gas constant (J/(kg*K)) = 8.314e-3 [J/kmol*K] / gas MW [kg/kmol] */

/* Initial pressure (Pa) - Example: 3 kgf/cm2 gauge pressure + atmospheric pressure (1 atm = 101325 Pa) */
real P_initial = 405300.0;

/* Global variables for time step calculation */
static real previous_time = 0.0;
static int first_call = 1;

/* Global variable to store updated pressure */
real P_updated = 405300.0;

/* Execute before each time step using UDF_ADJUST macro */
DEFINE_ADJUST(update_container_pressure, domain)
{
    /* Set initial mass on first execution */
    if (first_call)
    {
        /* Calculate initial mass using P = mRT/V */
        m_current = P_initial * V_container / (R_gas * T_container);
        previous_time = CURRENT_TIME;
        first_call = 0;
        return; /* Skip calculation on first call */
    }
    
    /* Calculate time step length */
    real current_time = CURRENT_TIME;
    real dt = current_time - previous_time;
    previous_time = current_time;
    
    if (dt > 0.0) /* Calculate from second time step onwards */
    {
        /* 1. Find outlet surface pointer (must match the name in Fluent) */
        Thread *t = Lookup_Thread(domain, "inlet_r51101_burst"); /* Change "outlet" name to actual boundary name */

        if (t != NULL)
        {
            /* 2. Calculate mass flow rate through outlet */
            /* Sum mass flow rates of each cell face */
            
            real mass_flow_rate = 0.0;
            face_t f;
            cell_t c0, c1;

            begin_f_loop(f, t)
            {
                /* Get adjacent cells */
                c0 = F_C0(f, t);
                c1 = F_C1(f, t);
                
                /* Calculate mass flux = density * velocity flux */
                if (c1 == -1) /* External boundary */
                {
                    real density = C_R(c0, THREAD_T0(t));
                    mass_flow_rate += density * F_FLUX(f, t);
                }
                else /* Internal face - should not occur for boundary */
                {
                    real density = 0.5 * (C_R(c0, THREAD_T0(t)) + C_R(c1, THREAD_T1(t)));
                    mass_flow_rate += density * F_FLUX(f, t);
                }
            }
            end_f_loop(f, t)
            
            /* 3. Update mass in container */
            /* Mass lost = flow rate * time step */
            /* Note: Positive flux means outflow from domain */
            m_current -= mass_flow_rate * dt;

            /* Correct to prevent mass from becoming negative (physical constraint) */
            if (m_current < 0.0) m_current = 0.0;
            
            /* 4. Calculate new container pressure */
            real P_new = (m_current * R_gas * T_container) / V_container;

            /* Update global pressure variable */
            P_updated = P_new;

            /* Print to console for debugging and monitoring */
            printf("\nTime: %f s, dt: %f s, Mass Flow Rate: %f kg/s, Current Mass: %f kg, New Pressure: %f Pa\n", 
                   current_time, dt, mass_flow_rate, m_current, P_new);
        }
    }
}

/* DEFINE_PROFILE UDF to set pressure boundary condition */
/* This should be assigned to the pressure inlet/outlet boundary in Fluent GUI */
DEFINE_PROFILE(pressure_profile, t, i)
{
    face_t f;
    
    begin_f_loop(f, t)
    {
        F_PROFILE(f, t, i) = P_updated;
    }
    end_f_loop(f, t)
}