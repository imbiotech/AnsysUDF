# 질량 유동 UDF에 Output Value 추가 예시

## 현재 코드에 모니터링 기능 추가

```c
#include "udf.h"
#include "sg_mphase.h"

/* Define global variables for initial mass and volume of the container */
real V_container = 10;
real m_current;
real T_container = 25+273.15;
real R_gas = 8.314*10^-3/88.15;
real P_initial = 405300.0;

/* Global variables for time step calculation */
static real previous_time = 0.0;
static int first_call = 1;

/* Global variable to store updated pressure */
real P_updated = 405300.0;

/* Global variables for output monitoring */
real total_mass_lost = 0.0;        // 총 손실 질량
real avg_mass_flow_rate = 0.0;     // 평균 질량 유량
real max_pressure = 0.0;           // 최대 압력
real min_pressure = 1e20;          // 최소 압력

DEFINE_ADJUST(update_container_pressure, domain)
{
#if !RP_HOST
    if (first_call)
    {
        m_current = P_initial * V_container / (R_gas * T_container);
        previous_time = CURRENT_TIME;
        first_call = 0;
        max_pressure = P_initial;
        min_pressure = P_initial;
        return;
    }
    
    real current_time = CURRENT_TIME;
    real dt = current_time - previous_time;
    previous_time = current_time;
    
    if (dt > 0.0)
    {
        Thread *t = Lookup_Thread(domain, "inlet_r51101_burst");
        
        if (t != NULL)
        {
            real mass_flow_rate = 0.0;
            face_t f;
            cell_t c0, c1;

            begin_f_loop(f, t)
            {
                c0 = F_C0(f, t);
                c1 = F_C1(f, t);
                
                if (c1 == -1)
                {
                    real density = C_R(c0, THREAD_T0(t));
                    mass_flow_rate += density * F_FLUX(f, t);
                }
            }
            end_f_loop(f, t)
            
            // 질량 및 압력 업데이트
            real mass_lost_this_step = mass_flow_rate * dt;
            m_current -= mass_lost_this_step;
            total_mass_lost += mass_lost_this_step;  // 누적 손실량
            
            if (m_current < 0.0) m_current = 0.0;
            
            real P_new = (m_current * R_gas * T_container) / V_container;
            P_updated = P_new;
            
            // 통계 업데이트
            if (P_new > max_pressure) max_pressure = P_new;
            if (P_new < min_pressure) min_pressure = P_new;
            
            // 이동 평균 계산 (간단한 버전)
            static real flow_rate_sum = 0.0;
            static int flow_count = 0;
            flow_rate_sum += mass_flow_rate;
            flow_count++;
            avg_mass_flow_rate = flow_rate_sum / flow_count;
            
            // UDM에 값들 저장 (후처리에서 확인 가능)
            // UDM 설정 필요: Define > User-Defined > Memory (최소 4개)
            Thread *fluid_thread;
            cell_t c;
            
            thread_loop_c(fluid_thread, domain)
            {
                if (FLUID_THREAD_P(fluid_thread))
                {
                    begin_c_loop(c, fluid_thread)
                    {
                        C_UDMI(c, fluid_thread, 0) = P_new;              // 현재 압력
                        C_UDMI(c, fluid_thread, 1) = m_current;          // 현재 질량
                        C_UDMI(c, fluid_thread, 2) = mass_flow_rate;     // 질량 유량
                        C_UDMI(c, fluid_thread, 3) = total_mass_lost;    // 총 손실량
                    }
                    end_c_loop(c, fluid_thread)
                }
            }
            
            printf("\nTime: %f s, Pressure: %f Pa, Mass: %f kg, Flow Rate: %f kg/s\n", 
                   current_time, P_new, m_current, mass_flow_rate);
        }
    }
#endif
}

/* 실시간 통계 확인 함수 */
DEFINE_EXECUTE_ON_DEMAND(container_statistics)
{
    printf("\n========== Container Statistics ==========\n");
    printf("Current Time: %f s\n", CURRENT_TIME);
    printf("Current Pressure: %f Pa (%.2f bar)\n", P_updated, P_updated/1e5);
    printf("Current Mass: %f kg\n", m_current);
    printf("Initial Mass: %f kg\n", P_initial * V_container / (R_gas * T_container));
    printf("Total Mass Lost: %f kg\n", total_mass_lost);
    printf("Mass Loss Percentage: %.2f%%\n", 
           total_mass_lost / (P_initial * V_container / (R_gas * T_container)) * 100.0);
    printf("Average Mass Flow Rate: %f kg/s\n", avg_mass_flow_rate);
    printf("Maximum Pressure: %f Pa\n", max_pressure);
    printf("Minimum Pressure: %f Pa\n", min_pressure);
    printf("Pressure Drop: %f Pa (%.2f%%)\n", 
           max_pressure - P_updated, 
           (max_pressure - P_updated) / max_pressure * 100.0);
    printf("=========================================\n");
}

/* 결과를 CSV 파일로 저장 */
DEFINE_EXECUTE_AT_END(save_container_data)
{
    static int first_save = 1;
    FILE *fp;
    
    if (first_save)
    {
        fp = fopen("container_results.csv", "w");
        fprintf(fp, "Time(s),Pressure(Pa),Mass(kg),TotalMassLost(kg),AvgFlowRate(kg/s)\n");
        first_save = 0;
    }
    else
    {
        fp = fopen("container_results.csv", "a");
    }
    
    if (fp != NULL)
    {
        fprintf(fp, "%f,%f,%f,%f,%f\n", 
                CURRENT_TIME, P_updated, m_current, total_mass_lost, avg_mass_flow_rate);
        fclose(fp);
    }
}

/* 특정 임계값 도달 시 알림 */
DEFINE_ADJUST(pressure_monitor, domain)
{
    static int warning_shown = 0;
    static int critical_shown = 0;
    
    real pressure_ratio = P_updated / P_initial;
    
    // 압력이 초기값의 50% 이하로 떨어지면 경고
    if (pressure_ratio < 0.5 && !warning_shown)
    {
        printf("\n*** WARNING: Pressure dropped below 50%% of initial value! ***\n");
        printf("Current pressure: %f Pa (%.1f%% of initial)\n", 
               P_updated, pressure_ratio * 100.0);
        warning_shown = 1;
    }
    
    // 압력이 초기값의 10% 이하로 떨어지면 위험 알림
    if (pressure_ratio < 0.1 && !critical_shown)
    {
        printf("\n*** CRITICAL: Pressure dropped below 10%% of initial value! ***\n");
        printf("Current pressure: %f Pa (%.1f%% of initial)\n", 
               P_updated, pressure_ratio * 100.0);
        critical_shown = 1;
    }
}

DEFINE_PROFILE(pressure_profile, t, i)
{
    face_t f;
    
    begin_f_loop(f, t)
    {
        F_PROFILE(f, t, i) = P_updated;
    }
    end_f_loop(f, t)
}
```

## 사용 방법:

### 1. Fluent 설정
```
Define > User-Defined > Memory... 
→ 4개 이상의 UDM 설정

Define > User-Defined > Functions > Compiled...
→ UDF 컴파일

Define > User-Defined > Execute On Demand
→ container_statistics 함수 등록
```

### 2. 모니터링
- **실시간**: 콘솔에서 매 스텝 출력 확인
- **통계**: Execute On Demand에서 `container_statistics` 실행
- **시각화**: Graphics > Contours에서 UDM-0~3 확인
- **데이터**: `container_results.csv` 파일 생성

### 3. 후처리에서 확인 가능한 값들
- UDM-0: 현재 압력
- UDM-1: 현재 질량  
- UDM-2: 질량 유량
- UDM-3: 총 손실 질량

이렇게 하면 UDF의 계산 결과를 다양한 방식으로 모니터링하고 출력할 수 있습니다!
