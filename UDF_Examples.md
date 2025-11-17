# UDF 예제 모음

## 1. 간단한 온도 프로파일 설정

```c
#include "udf.h"

DEFINE_PROFILE(inlet_temperature, t, i)
{
    face_t f;
    real x[ND_ND];  // 좌표 배열
    
    begin_f_loop(f, t)
    {
        F_CENTROID(x, f, t);  // 면의 중심 좌표 얻기
        
        // y 좌표에 따른 온도 분포
        F_PROFILE(f, t, i) = 300.0 + 50.0 * x[1];  // T = 300 + 50*y
    }
    end_f_loop(f, t)
}
```

## 2. 시간에 따른 압력 변화

```c
#include "udf.h"

DEFINE_PROFILE(time_varying_pressure, t, i)
{
    face_t f;
    real current_time = CURRENT_TIME;
    
    begin_f_loop(f, t)
    {
        // 시간에 따른 정현파 압력
        F_PROFILE(f, t, i) = 101325.0 + 1000.0 * sin(2.0 * M_PI * current_time);
    }
    end_f_loop(f, t)
}
```

## 3. 속도 프로파일 (포물선형)

```c
#include "udf.h"

DEFINE_PROFILE(parabolic_velocity, t, i)
{
    face_t f;
    real x[ND_ND];
    real y, v_max = 2.0;  // 최대 속도 2 m/s
    
    begin_f_loop(f, t)
    {
        F_CENTROID(x, f, t);
        y = x[1];  // y 좌표
        
        // 포물선형 속도 분포: v = v_max * (1 - (y/0.5)^2)
        F_PROFILE(f, t, i) = v_max * (1.0 - pow(y/0.5, 2.0));
    }
    end_f_loop(f, t)
}
```

## 4. 열원 추가

```c
#include "udf.h"

DEFINE_SOURCE(heat_source, c, t, dS, eqn)
{
    real source = 1000.0;  // W/m³
    real x[ND_ND];
    
    C_CENTROID(x, c, t);
    
    // 특정 영역에만 열원 적용
    if (x[0] > 0.1 && x[0] < 0.2)
    {
        dS[eqn] = 0.0;  // 소스의 미분값
        return source;
    }
    
    dS[eqn] = 0.0;
    return 0.0;
}
```

## 5. 밀도 계산 (이상기체)

```c
#include "udf.h"

DEFINE_PROPERTY(ideal_gas_density, c, t)
{
    real T = C_T(c, t);        // 온도
    real P = C_P(c, t);        // 압력
    real R = 287.0;            // 공기의 기체 상수 J/(kg*K)
    
    return P / (R * T);        // ρ = P/(RT)
}
```

## 6. 점성계수 계산 (Sutherland 공식)

```c
#include "udf.h"

DEFINE_PROPERTY(sutherland_viscosity, c, t)
{
    real T = C_T(c, t);
    real mu0 = 1.716e-5;       // 참조 점성계수 (Pa*s)
    real T0 = 273.15;          // 참조 온도 (K)
    real S = 110.4;            // Sutherland 상수 (K)
    
    return mu0 * pow(T/T0, 1.5) * (T0 + S) / (T + S);
}
```

## 7. 벽면 열유속 계산

```c
#include "udf.h"

DEFINE_PROFILE(wall_heat_flux, t, i)
{
    face_t f;
    real T_wall = 350.0;       // 벽면 온도
    real h = 25.0;             // 열전달 계수 W/(m²*K)
    cell_t c0;
    
    begin_f_loop(f, t)
    {
        c0 = F_C0(f, t);
        real T_fluid = C_T(c0, THREAD_T0(t));
        
        // 뉴턴의 냉각 법칙: q = h*(T_wall - T_fluid)
        F_PROFILE(f, t, i) = h * (T_wall - T_fluid);
    }
    end_f_loop(f, t)
}
```

## 8. 사용자 정의 스칼라 초기화

```c
#include "udf.h"

DEFINE_INIT(initialize_scalar, domain)
{
    cell_t c;
    Thread *t;
    real x[ND_ND];
    
    thread_loop_c(t, domain)
    {
        begin_c_loop(c, t)
        {
            C_CENTROID(x, c, t);
            
            // 위치에 따른 스칼라 초기값 설정
            C_UDSI(c, t, 0) = x[0] + x[1];  // x + y
        }
        end_c_loop(c, t)
    }
}
```

## 9. 질량 모니터링

```c
#include "udf.h"

DEFINE_EXECUTE_AT_END(mass_monitor)
{
    Domain *d = Get_Domain(1);
    Thread *t;
    cell_t c;
    real total_mass = 0.0;
    real volume, density;
    
    thread_loop_c(t, d)
    {
        if (FLUID_THREAD_P(t))  // 유체 영역만
        {
            begin_c_loop(c, t)
            {
                volume = C_VOLUME(c, t);
                density = C_R(c, t);
                total_mass += density * volume;
            }
            end_c_loop(c, t)
        }
    }
    
    printf("Total mass: %f kg\n", total_mass);
}
```

## 10. 평균값 계산

```c
#include "udf.h"

DEFINE_EXECUTE_ON_DEMAND(calculate_average_temperature)
{
    Domain *d = Get_Domain(1);
    Thread *t;
    cell_t c;
    real sum_temp = 0.0;
    real total_volume = 0.0;
    real volume, temperature;
    
    thread_loop_c(t, d)
    {
        if (FLUID_THREAD_P(t))
        {
            begin_c_loop(c, t)
            {
                volume = C_VOLUME(c, t);
                temperature = C_T(c, t);
                
                sum_temp += temperature * volume;
                total_volume += volume;
            }
            end_c_loop(c, t)
        }
    }
    
    real avg_temp = sum_temp / total_volume;
    printf("Average Temperature: %f K\n", avg_temp);
}
```

## 11. 경계면에서의 평균 계산

```c
#include "udf.h"

DEFINE_EXECUTE_ON_DEMAND(boundary_average)
{
    Domain *d = Get_Domain(1);
    Thread *t = Lookup_Thread(d, "outlet");  // 출구 경계면
    face_t f;
    real sum_pressure = 0.0;
    real total_area = 0.0;
    real area, pressure;
    cell_t c0;
    
    if (t != NULL)
    {
        begin_f_loop(f, t)
        {
            c0 = F_C0(f, t);
            area = F_AREA(f, t);
            pressure = C_P(c0, THREAD_T0(t));
            
            sum_pressure += pressure * area;
            total_area += area;
        }
        end_f_loop(f, t)
        
        real avg_pressure = sum_pressure / total_area;
        printf("Average outlet pressure: %f Pa\n", avg_pressure);
    }
}
```

## 12. 파일에 데이터 저장

```c
#include "udf.h"

DEFINE_EXECUTE_AT_END(save_data)
{
    FILE *fp;
    static int first_time = 1;
    
    if (first_time)
    {
        fp = fopen("results.txt", "w");
        fprintf(fp, "Time(s)\tPressure(Pa)\tTemperature(K)\n");
        first_time = 0;
    }
    else
    {
        fp = fopen("results.txt", "a");
    }
    
    if (fp != NULL)
    {
        fprintf(fp, "%f\t%f\t%f\n", CURRENT_TIME, 101325.0, 300.0);
        fclose(fp);
    }
}
```

## 사용법 요약

### 1. 컴파일 및 로드
- `Define > User-Defined > Functions > Compiled...`
- Source Files에 .c 파일 추가
- Build 클릭

### 2. 함수 할당
- **DEFINE_PROFILE**: 경계 조건에서 선택
- **DEFINE_SOURCE**: 셀 존에서 소스 항으로 선택
- **DEFINE_EXECUTE_ON_DEMAND**: Execute Commands에서 실행
- **DEFINE_EXECUTE_AT_END**: 자동으로 매 반복 후 실행

### 3. 디버깅
- `printf()` 함수로 값 확인
- Fluent 콘솔에서 출력 확인
- 파일로 데이터 저장해서 분석

이 예제들을 참고하시면 다양한 UDF를 작성하실 수 있을 것입니다!
