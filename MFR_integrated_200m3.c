#include "udf.h"

/*
 * ANSYS UDF: Inlet 경계면의 유속 계산 함수
 * 
 * Transient 해석에서 유체 질량 감소에 따라 inlet 유속을 계산
 */

/* 전역 변수: 유체의 초기값 및 경계면 정보 */
static real initial_mass = 0.0;             /* 유체의 최초 질량 (kg) */
static real initial_supply_pressure = 0.0; /* 유체의 최초 공급압 (Pa) */
static char inlet_boundary_name[256] = "";  /* 경계면의 이름 */

static real current_total_mass = 0.0;       /* 현재 유체의 총량 (kg) */

/*
 * 함수: Calculate_Inlet_Mass_Flow_Rate
 * 목적: inlet 경계면의 질량 유량을 유체 질량에 비례해서 계산 (transient 해석)
 * 
 * 반환값: 계산된 inlet 질량 유량 (kg/s)
 * 
 * 계산식:
 * new_mass_flow_rate = base_mass_flow_rate * (current_total_mass / initial_mass)
 */
real Calculate_Inlet_Mass_Flow_Rate(
    real total_mass,              /* 현재 유체의 총량 (kg) */
    real base_mass_flow_rate)     /* 기본 inlet 질량 유량 (kg/s) */
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
 * 함수: DEFINE_PROFILE (inlet_mass_flow_rate_profile)
 * 목적: inlet 경계면에서의 질량 유량 프로파일 정의
 */
DEFINE_PROFILE(inlet_mass_flow_rate_profile, thread, position)
{
    real mass_flow_rate;
    face_t f;
    
    /* *** [필수 입력] 기본 inlet 질량 유량 (kg/s) - 사용자가 직접 입력 *** */
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
 * 함수: Set_Total_Mass
 * 목적: 매 time step마다 현재 유체 총량 업데이트
 */
void Set_Total_Mass(real mass)
{
    current_total_mass = mass;
}

/*
 * 함수: Get_Inlet_Mass_Flow_Rate
 * 목적: 계산된 inlet 질량 유량 조회
 */
real Get_Inlet_Mass_Flow_Rate(real base_mass_flow_rate)
{
    return Calculate_Inlet_Mass_Flow_Rate(current_total_mass, base_mass_flow_rate);
}

/*
 * 함수: Reset_Total_Mass
 * 목적: 해석 시작 전 초기화
 */
void Reset_Total_Mass(void)
{
    current_total_mass = 0.0;
}

/*
 * 함수: Set_Initial_Parameters
 * 목적: 초기 파라미터 설정
 * 
 * *** [필수 입력] 해석 시작 전에 실제 값으로 호출 ***
 * 예시: Set_Initial_Parameters(100.0, 101325.0, "inlet");
 *       - 첫번째 인자: 유체의 최초 질량 (kg)
 *       - 두번째 인자: 유체의 최초 공급압 (Pa)
 *       - 세번째 인자: 경계면의 이름 (문자열)
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
 * 함수: Get_Initial_Mass
 * 목적: 유체의 최초 질량 조회
 * 
 * 반환값: 유체의 최초 질량 (kg)
 */
real Get_Initial_Mass(void)
{
    return initial_mass;
}

/*
 * 함수: Get_Initial_Supply_Pressure
 * 목적: 유체의 최초 공급압 조회
 * 
 * 반환값: 유체의 최초 공급압 (Pa)
 */
real Get_Initial_Supply_Pressure(void)
{
    return initial_supply_pressure;
}

/*
 * 함수: Get_Inlet_Boundary_Name
 * 목적: 경계면의 이름 조회
 * 
 * 반환값: 경계면의 이름 (문자열 포인터)
 */
const char* Get_Inlet_Boundary_Name(void)
{
    return inlet_boundary_name;
}
