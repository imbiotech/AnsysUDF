# Ansys Fluent UDF 기본 가이드

## 1. C 언어 기본 (Python/C# 대비)

### 1.1 변수 선언
```c
// C 언어
real temperature = 300.0;      // 실수형 변수
int count = 10;               // 정수형 변수
static real value = 0.0;      // 정적 변수 (프로그램 종료까지 유지)

// Python 비교
// temperature = 300.0
// count = 10

// C# 비교
// double temperature = 300.0;
// int count = 10;
// static double value = 0.0;
```

### 1.2 포인터 (Python에는 없는 개념)
```c
Thread *t;              // Thread 타입의 포인터 변수
*t                      // 포인터가 가리키는 값
&variable              // 변수의 주소
```

### 1.3 전처리기 지시문
```c
#include "udf.h"       // 헤더 파일 포함 (Python의 import와 유사)
#if !RP_HOST          // 조건부 컴파일 (병렬 처리 관련)
#endif
```

## 2. UDF 특화 데이터 타입

### 2.1 기본 데이터 타입
```c
real        // 실수형 (double과 유사)
face_t      // 면(face) ID
cell_t      // 셀(cell) ID
Thread*     // 경계면이나 셀 영역 포인터
Domain*     // 도메인 포인터
```

### 2.2 Fluent 매크로
```c
// 시간 관련
CURRENT_TIME           // 현재 시뮬레이션 시간

// 면(Face) 관련
F_FLUX(f, t)          // 면 f에서의 부피 유량
F_C0(f, t)            // 면 f의 첫 번째 인접 셀
F_C1(f, t)            // 면 f의 두 번째 인접 셀 (-1이면 경계면)

// 셀(Cell) 관련
C_R(c, t)             // 셀 c의 밀도
C_T(c, t)             // 셀 c의 온도
C_P(c, t)             // 셀 c의 압력

// 스레드 관련
THREAD_T0(t)          // 스레드 t의 첫 번째 셀 스레드
THREAD_T1(t)          // 스레드 t의 두 번째 셀 스레드
```

## 3. UDF 매크로 종류

### 3.1 DEFINE_ADJUST
```c
DEFINE_ADJUST(function_name, domain)
{
    // 매 타임 스텝 시작 전에 실행
    // 전역 변수 업데이트, 계산 등에 사용
}
```

### 3.2 DEFINE_PROFILE
```c
DEFINE_PROFILE(function_name, thread, index)
{
    // 경계 조건 설정에 사용
    // 압력, 온도, 속도 등을 동적으로 설정
    face_t f;
    begin_f_loop(f, thread)
    {
        F_PROFILE(f, thread, index) = value;
    }
    end_f_loop(f, thread)
}
```

### 3.3 DEFINE_SOURCE
```c
DEFINE_SOURCE(function_name, cell, thread, dS, eqn)
{
    // 소스 항 추가 (질량, 운동량, 에너지 등)
    return source_value;
}
```

### 3.4 DEFINE_PROPERTY
```c
DEFINE_PROPERTY(function_name, cell, thread)
{
    // 물성치를 동적으로 계산
    return property_value;
}
```

## 4. 루프 구조

### 4.1 면(Face) 루프
```c
face_t f;
begin_f_loop(f, thread)
{
    // 각 면에 대해 반복 실행
    // f는 현재 면의 ID
}
end_f_loop(f, thread)

// Python의 for문과 비교
// for f in faces:
//     # 각 면에 대한 작업
```

### 4.2 셀(Cell) 루프
```c
cell_t c;
begin_c_loop(c, thread)
{
    // 각 셀에 대해 반복 실행
}
end_c_loop(c, thread)
```

## 5. 병렬 처리 관련

### 5.1 병렬 처리 매크로
```c
#if !RP_HOST          // 계산 노드에서만 실행
    // 계산 코드
#endif

#if RP_NODE           // 병렬 노드에서만 실행
    // 데이터 동기화 코드
#endif

// 데이터 동기화
PRF_GRSUM1(value)     // 모든 노드의 값을 합산
PRF_GRHIGH1(value)    // 모든 노드 중 최댓값
PRF_GRLOW1(value)     // 모든 노드 중 최솟값
```

## 6. 일반적인 UDF 패턴

### 6.1 초기화 패턴
```c
static int first_call = 1;

DEFINE_ADJUST(my_function, domain)
{
    if (first_call)
    {
        // 초기화 코드
        first_call = 0;
        return;
    }
    
    // 메인 계산 코드
}
```

### 6.2 시간 스텝 계산
```c
static real previous_time = 0.0;

DEFINE_ADJUST(my_function, domain)
{
    real current_time = CURRENT_TIME;
    real dt = current_time - previous_time;
    previous_time = current_time;
    
    if (dt > 0.0)
    {
        // 시간 스텝 기반 계산
    }
}
```

### 6.3 경계면 찾기
```c
Thread *t = Lookup_Thread(domain, "boundary_name");
if (t != NULL)
{
    // 경계면이 존재할 때의 처리
}
```

## 7. 컴파일 및 로딩

### 7.1 컴파일 방법
1. Fluent에서 `Define > User-Defined > Functions > Compiled...`
2. Source Files에 .c 파일 추가
3. `Build` 클릭

### 7.2 UDF 사용법
1. **DEFINE_ADJUST**: `Define > User-Defined > Execute On Demand`에서 활성화
2. **DEFINE_PROFILE**: 경계 조건 설정 시 선택
3. **DEFINE_SOURCE**: 셀 존 조건에서 소스 항으로 설정

## 8. 디버깅 팁

### 8.1 출력 함수
```c
printf("값: %f\n", value);           // 실수 출력
printf("정수: %d\n", count);         // 정수 출력
printf("문자열: %s\n", "Hello");     // 문자열 출력

// Python 비교: print(f"값: {value}")
```

### 8.2 조건부 출력
```c
#if DEBUG
    printf("디버그: %f\n", value);
#endif
```

## 9. 에러 방지

### 9.1 NULL 체크
```c
Thread *t = Lookup_Thread(domain, "boundary_name");
if (t != NULL)  // 항상 NULL 체크
{
    // 안전한 코드
}
```

### 9.2 나누기 에러 방지
```c
if (denominator != 0.0)
{
    result = numerator / denominator;
}
```

### 9.3 배열 범위 확인
```c
if (index >= 0 && index < array_size)
{
    // 안전한 배열 접근
}
```

## 10. 실제 예제 분석

현재 코드에서 핵심 부분들:

```c
// 1. 전역 변수 선언
real V_container = 10;      // 용기 부피

// 2. 초기화
if (first_call)
{
    m_current = P_initial * V_container / (R_gas * T_container);
    first_call = 0;
    return;
}

// 3. 시간 스텝 계산
real dt = current_time - previous_time;

// 4. 경계면 찾기 및 유량 계산
Thread *t = Lookup_Thread(domain, "inlet_r51101_burst");
begin_f_loop(f, t)
{
    mass_flow_rate += density * F_FLUX(f, t);
}
end_f_loop(f, t)

// 5. 물리 법칙 적용 (이상기체 법칙)
real P_new = (m_current * R_gas * T_container) / V_container;
```

이 가이드를 참고하시면 UDF 코드를 훨씬 쉽게 이해하실 수 있을 것입니다!

## 11. Output Value 관리 방법

### 11.1 User-Defined Memory (UDM) 사용
```c
// UDM을 사용해서 계산 결과를 저장하고 후처리에서 활용
DEFINE_ADJUST(calculate_values, domain)
{
    Thread *t;
    cell_t c;
    
    thread_loop_c(t, domain)
    {
        if (FLUID_THREAD_P(t))
        {
            begin_c_loop(c, t)
            {
                real temperature = C_T(c, t);
                real pressure = C_P(c, t);
                
                // 사용자 정의 계산 (예: 엔탈피)
                real enthalpy = temperature * 1005.0;  // Cp * T
                
                // UDM에 저장 (UDM-0에 저장)
                C_UDMI(c, t, 0) = enthalpy;
                
                // 여러 값 저장 가능 (UDM-1, UDM-2...)
                C_UDMI(c, t, 1) = pressure / temperature;  // 밀도 비례값
            }
            end_c_loop(c, t)
        }
    }
}
```

### 11.2 User-Defined Scalar (UDS) 사용
```c
// UDS를 사용해서 transport equation과 함께 해결
DEFINE_SOURCE(custom_scalar_source, c, t, dS, eqn)
{
    real source_value = 0.0;
    real temperature = C_T(c, t);
    
    // 온도에 따른 소스 항 계산
    source_value = 1000.0 * exp(-temperature / 300.0);
    
    dS[eqn] = -1000.0 / 300.0 * exp(-temperature / 300.0);  // 미분값
    
    return source_value;
}

// UDS 값에 접근
DEFINE_ADJUST(monitor_scalar, domain)
{
    Thread *t;
    cell_t c;
    real max_scalar = 0.0;
    
    thread_loop_c(t, domain)
    {
        if (FLUID_THREAD_P(t))
        {
            begin_c_loop(c, t)
            {
                real scalar_value = C_UDSI(c, t, 0);  // UDS-0 값
                if (scalar_value > max_scalar)
                    max_scalar = scalar_value;
            }
            end_c_loop(c, t)
        }
    }
    
    printf("Maximum scalar value: %f\n", max_scalar);
}
```

### 11.3 파일로 결과 출력
```c
// 계산 결과를 파일에 저장
DEFINE_EXECUTE_AT_END(save_results)
{
    FILE *fp;
    Domain *d = Get_Domain(1);
    Thread *t;
    cell_t c;
    real x[ND_ND];
    static int time_step = 0;
    
    char filename[100];
    sprintf(filename, "results_step_%d.csv", time_step++);
    
    fp = fopen(filename, "w");
    if (fp != NULL)
    {
        fprintf(fp, "X,Y,Z,Temperature,Pressure,Custom_Value\n");
        
        thread_loop_c(t, d)
        {
            if (FLUID_THREAD_P(t))
            {
                begin_c_loop(c, t)
                {
                    C_CENTROID(x, c, t);
                    real temp = C_T(c, t);
                    real press = C_P(c, t);
                    real custom = temp * press / 287.0;  // 밀도 계산
                    
                    fprintf(fp, "%f,%f,%f,%f,%f,%f\n", 
                           x[0], x[1], x[2], temp, press, custom);
                }
                end_c_loop(c, t)
            }
        }
        fclose(fp);
    }
}
```

### 11.4 DEFINE_ON_DEMAND로 실시간 모니터링
```c
// 사용자가 원할 때 실행해서 결과 확인
DEFINE_EXECUTE_ON_DEMAND(calculate_custom_outputs)
{
    Domain *d = Get_Domain(1);
    Thread *t;
    cell_t c;
    real sum_value = 0.0;
    real max_value = -1e20;
    real min_value = 1e20;
    int cell_count = 0;
    
    thread_loop_c(t, d)
    {
        if (FLUID_THREAD_P(t))
        {
            begin_c_loop(c, t)
            {
                // 사용자 정의 계산
                real temperature = C_T(c, t);
                real velocity_mag = sqrt(pow(C_U(c, t), 2) + 
                                       pow(C_V(c, t), 2) + 
                                       pow(C_W(c, t), 2));
                real reynolds_cell = velocity_mag * 0.01 / 1e-5;  // Re 계산
                
                sum_value += reynolds_cell;
                if (reynolds_cell > max_value) max_value = reynolds_cell;
                if (reynolds_cell < min_value) min_value = reynolds_cell;
                cell_count++;
                
                // UDM에 저장 (후처리에서 볼 수 있음)
                C_UDMI(c, t, 0) = reynolds_cell;
            }
            end_c_loop(c, t)
        }
    }
    
    real avg_value = sum_value / cell_count;
    printf("\n=== Custom Output Statistics ===\n");
    printf("Average Reynolds Number: %f\n", avg_value);
    printf("Maximum Reynolds Number: %f\n", max_value);
    printf("Minimum Reynolds Number: %f\n", min_value);
    printf("Total Cells: %d\n", cell_count);
}
```

### 11.5 특정 위치/경계면에서의 값 출력
```c
// 특정 경계면에서의 평균값 계산
DEFINE_EXECUTE_ON_DEMAND(boundary_output)
{
    Domain *d = Get_Domain(1);
    Thread *t = Lookup_Thread(d, "outlet");
    face_t f;
    real sum_pressure = 0.0;
    real sum_velocity = 0.0;
    real total_area = 0.0;
    cell_t c0;
    
    if (t != NULL)
    {
        begin_f_loop(f, t)
        {
            c0 = F_C0(f, t);
            real area = F_AREA(f, t);
            real pressure = C_P(c0, THREAD_T0(t));
            real u_vel = C_U(c0, THREAD_T0(t));
            real v_vel = C_V(c0, THREAD_T0(t));
            real velocity_mag = sqrt(u_vel*u_vel + v_vel*v_vel);
            
            sum_pressure += pressure * area;
            sum_velocity += velocity_mag * area;
            total_area += area;
        }
        end_f_loop(f, t)
        
        printf("\n=== Outlet Boundary Results ===\n");
        printf("Area-averaged pressure: %f Pa\n", sum_pressure/total_area);
        printf("Area-averaged velocity: %f m/s\n", sum_velocity/total_area);
        printf("Total area: %f m²\n", total_area);
    }
}
```

## 사용 방법:

### 1. UDM 설정 (Fluent GUI에서)
- `Define > User-Defined > Memory...`
- 필요한 만큼 UDM 개수 설정 (예: 5개)

### 2. UDS 설정
- `Define > User-Defined > Scalars...`
- 필요한 UDS 추가하고 transport equation 활성화

### 3. 후처리에서 확인
- `Graphics > Contours`에서 UDM/UDS 선택해서 시각화
- `Reports > Volume Integrals`에서 통계 확인

### 4. 실시간 모니터링
- `Define > User-Defined > Execute On Demand`에서 함수 실행
