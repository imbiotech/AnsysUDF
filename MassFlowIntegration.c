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

#if !RP_HOST
/* Parallel processing macros */
#endif

/* Execute before each time step using UDF_ADJUST macro */
DEFINE_ADJUST(update_container_pressure, domain) {
    /* Always print basic info to verify UDF is running */
    printf("=== UDF ADJUST called at time: %f ===\n", CURRENT_TIME);
    fflush(stdout); /* Force output buffer flush */

#if !RP_HOST
    /* Set initial mass on first execution */
    if (first_call) {
        /* Calculate initial mass using P = mRT/V */
        m_current = P_initial * V_container / (R_gas * T_container);
        previous_time = CURRENT_TIME;
        first_call = 0;
        printf("*** FIRST CALL - Initial mass: %f kg, Initial pressure: %f Pa ***\n", 
               m_current, P_initial);
        fflush(stdout);
        return; /* Skip calculation on first call */
    }
    
    /* Calculate time step length */
    real current_time = CURRENT_TIME;
    real dt = current_time - previous_time;
    previous_time = current_time;
    
    printf("Time step info - Current time: %f, dt: %f\n", current_time, dt);
    fflush(stdout);
    
    if (dt > 0.0) /* Calculate from second time step onwards */ {
        /* 1. Find rupture disk inlet surface (gas flowing from container to atmosphere) */
        Thread *t = Lookup_Thread(domain, "inlet_r51101_burst"); /* Rupture disk inlet boundary */

        printf("Looking for thread 'inlet_r51101_burst'...\n");
        fflush(stdout);

        if (t != NULL) {
            printf("*** Thread found! Calculating mass flow rate ***\n");
            fflush(stdout);
            /* 2. Calculate mass flow rate through rupture disk inlet */
            /* This represents gas flowing OUT of the container (INTO the CFD domain) */
            
            real mass_flow_rate = 0.0;
            face_t f;

            begin_f_loop(f, t) {
                /* For inlet boundary, use boundary face density and flux */
                real density = F_R(f, t);  /* Face density at inlet boundary */
                real velocity_flux = F_FLUX(f, t); /* Volume flux (m3/s) - positive for inlet */
                
                /* Add to total mass flow rate */
                mass_flow_rate += density * velocity_flux;
            } end_f_loop(f, t)
            
            /* 3. Update mass in container */
            /* For rupture disk inlet: positive flux = gas leaving container (mass loss) */
            /* Mass leaving container = mass entering CFD domain through inlet */
            m_current -= mass_flow_rate * dt;

            /* Debug output to check flow direction */
            printf("Rupture disk mass flow rate: %e kg/s (positive = gas leaving container)\n", mass_flow_rate);
            fflush(stdout);

            /* Correct to prevent mass from becoming negative (physical constraint) */
            if (m_current < 0.0) {
                printf("Warning: Mass became negative, setting to zero\n");
                fflush(stdout);
                m_current = 0.0;
            }
            
            /* 4. Calculate new container pressure */
            real P_new = (m_current * R_gas * T_container) / V_container;
            
            /* Rupture disk behavior: */
            /* - If pressure drops below atmospheric, no more flow (check valve effect) */
            /* - In reality, the disk would reseal or flow would stop */
            real P_atm = 101325.0; /* Atmospheric pressure (Pa) */
            
            if (P_new <= P_atm) {
                /* Pressure reached or below atmospheric level - equilibrium achieved */
                P_new = P_atm;
                printf("*** EQUILIBRIUM REACHED - Container pressure at atmospheric level ***\n");
                fflush(stdout);
                
                /* Optional: Set mass flow to zero in inlet boundary condition */
                /* This would require modifying the inlet boundary condition in Fluent */
            }

            /* Update global pressure variable */
            P_updated = P_new;

            /* Enhanced debugging output */
            printf("\n=== CALCULATION RESULTS ===\n");
            printf("Time: %f s, dt: %f s\n", current_time, dt);
            printf("Mass Flow Rate: %e kg/s, Current Mass: %f kg\n", mass_flow_rate, m_current);
            printf("Calculated Pressure: %f Pa (%.2f bar), Atmospheric: %f Pa\n", 
                   P_new, P_new/100000.0, P_atm);
            printf("============================\n\n");
            fflush(stdout);
            
            /* Check if we're close to equilibrium */
            if (fabs(P_new - P_atm) < 1000.0) /* Within 1000 Pa of atmospheric */ {
                printf("*** APPROACHING EQUILIBRIUM - Pressure difference: %f Pa ***\n", P_new - P_atm);
                fflush(stdout);
            }
        }
        else {
            printf("ERROR: Thread 'inlet_r51101_burst' not found!\n");
            printf("Available threads should be checked in Fluent boundary conditions.\n");
            fflush(stdout);
        }
    }
    else {
        printf("Skipping calculation - dt = %f (should be > 0)\n", dt);
        fflush(stdout);
    }
    }
#endif /* !RP_HOST */
    
    /* Synchronize pressure value across all nodes */
#if RP_NODE
    P_updated = PRF_GRSUM1(P_updated);
#endif
}

/* DEFINE_PROFILE UDF to set pressure boundary condition */
/* This should be assigned to the pressure inlet/outlet boundary in Fluent GUI */
DEFINE_PROFILE(pressure_profile, t, i) {
    face_t f;
    
    begin_f_loop(f, t) {
        F_PROFILE(f, t, i) = P_updated;
    }
    end_f_loop(f, t)
}