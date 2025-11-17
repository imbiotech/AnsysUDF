#include "udf.h"
#include "sg_mphase.h"

/* 글로벌 변수로 용기의 초기 질량과 부피를 정의합니다. */
/* 실제 값에 맞게 수정하세요 */
real V_container = 10;      /* 용기 부피 (m^3) */
real m_current;              /* 현재 용기 내 질량 (kg) - 전역 변수로 선언 */
real T_container = 25+273.15;    /* 용기 온도 (K) - 등온 가정 */
real R_gas = 8.314*10^-3/88.15;          /* 기체 상수 (J/(kg*K)) = 8.314e-3 [J/kmol*K] / gas MW [kg/kmol] */

/* 초기 압력 (Pa) - 예시: 3 kgf/cm2 게이지 압력 + 대기압 (1 atm = 101325 pa) */
real P_initial = 405300.0;

/* UDF_ADJUST 매크로를 사용하여 매 타임 스텝 시작 전 실행 */
DEFINE_ADJUST(update_container_pressure, domain)
{
    /* 최초 실행 시 초기 질량 설정 */
    if (strcmp(string_var("case-name"), "initialized") != 0)
    {
        /* P = mRT/V 를 이용해 초기 질량 계산 */
        m_current = P_initial * V_container / (R_gas * T_container);
        /* 초기화 플래그 설정 (재초기화 방지) */
        host_to_node_string_var_set("case-name", "initialized");
    }
    
    /* 현재 타임 스텝 길이 가져오기 (비정상 해석 시 필요) */
    real dt = NV_MAG(SG_GTIME);
    
    if (dt > 0.0) /* 첫 타임 스텝 이후부터 계산 */
    {
        /* 1. 출구 표면 포인터 찾기 (Fluent 내 이름과 일치해야 함) */
        Thread *t = Lookup_Thread(domain, "outlet"); /* "outlet" 이름을 실제 경계면 이름으로 변경하세요 */

        if (t != NULL)
        {
            /* 2. 출구를 통한 질량 유량 계산 (Surface Integral 함수 사용) */
            /* Compute_Surface_Integral 함수는 UDF 내에서 사용 불가능 */
            /* 따라서 C_FLUX 매크로를 이용하여 각 셀 페이스의 유량 합산 */
            
            real mass_flow_rate = 0.0;
            face_t f;

            begin_f_loop(f, t)
            {
                /* F_FLUX 매크로는 질량 유량 (kg/s)을 반환 */
                /* Fluent는 내부적으로 유출을 양수로 처리합니다. */
                mass_flow_rate += F_FLUX(f, t);
            }
            end_f_loop(f, t)
            
            /* 3. 용기 내 질량 업데이트 */
            /* 빠져나간 질량 = 유량 * 타임 스텝 */
            m_current -= mass_flow_rate * dt;

            /* 질량이 0보다 작아지지 않도록 보정 (물리적 제한) */
            if (m_current < 0.0) m_current = 0.0;
        }

        /* 4. 새로운 용기 압력 계산 */
        real P_new = (m_current * R_gas * T_container) / V_container;

        /* 5. 새로운 압력을 경계 조건에 적용 */
        /* Lookup_Thread를 다시 사용하여 경계면의 압력을 설정 */
        Thread *t_bc = Lookup_Thread(domain, "outlet"); /* 동일한 "outlet" 이름 사용 */
        if (t_bc != NULL)
        {
            /* 모든 페이스에 새로운 압력 경계 조건 설정 */
            begin_f_loop(f, t_bc)
            {
                /* F_PROFILE 매크로를 사용하여 압력 경계 조건 설정 */
                /* 이 UDF는 압력 입구/출구 조건(Pressure Inlet/Outlet)에 적용되어야 함 */
                F_PROFILE(f, t_bc, SV_P) = P_new; 
            }
            end_f_loop(f, t_bc)
        }

        /* 디버깅 및 모니터링을 위해 콘솔에 출력 */
        printf("\nTime Step: %f, Mass Flow Rate: %f kg/s, New Pressure: %f Pa\n", 
               rp_time, mass_flow_rate, P_new);
    }
}