# 현재 UDF 코드 상세 분석

## 코드 목적
용기에서 기체가 배출되면서 용기 내부 압력이 변화하는 것을 시뮬레이션하는 UDF입니다.

## 1. 헤더 및 전역 변수

```c
#include "udf.h"           // Fluent UDF 함수들
#include "sg_mphase.h"     // 다상 유동 관련 함수들

// 물리적 상수들
real V_container = 10;              // 용기 부피 (m³)
real m_current;                     // 현재 용기 내 질량 (kg)
real T_container = 25+273.15;       // 용기 온도 (K) = 25°C를 절대온도로
real R_gas = 8.314*10^-3/88.15;     // 기체 상수 (J/(kg*K))
real P_initial = 405300.0;          // 초기 압력 (Pa)
```

**Python/C#과 비교:**
- C#: `double V_container = 10;`
- Python: `V_container = 10`

## 2. 제어 변수들

```c
static real previous_time = 0.0;    // 이전 시간 스텝 시간 저장
static int first_call = 1;          // 첫 실행 여부 확인 플래그
real P_updated = 405300.0;          // 업데이트된 압력값 저장
```

**`static` 키워드:**
- C#의 `static`과 동일
- 함수가 끝나도 값이 유지됨
- 프로그램 종료까지 메모리에 남아있음

## 3. 병렬 처리 관련

```c
#if !RP_HOST
/* Parallel processing macros */
#endif
```

**의미:**
- `!RP_HOST`: "호스트가 아닌 경우"
- 병렬 처리에서 계산 노드에서만 실행되도록 함
- 호스트는 관리용, 노드는 계산용

## 4. 메인 함수 - DEFINE_ADJUST

```c
DEFINE_ADJUST(update_container_pressure, domain)
{
```

**함수 역할:**
- 매 타임 스텝마다 자동 실행
- `update_container_pressure`: 함수 이름 (원하는 대로 변경 가능)
- `domain`: Fluent에서 제공하는 전체 계산 영역 정보

## 5. 초기화 섹션

```c
if (first_call)
{
    m_current = P_initial * V_container / (R_gas * T_container);
    previous_time = CURRENT_TIME;
    first_call = 0;
    return;
}
```

**단계별 설명:**
1. `first_call`이 1이면 (첫 실행)
2. **이상기체 법칙 사용**: `PV = mRT` → `m = PV/RT`
3. `CURRENT_TIME`: Fluent가 제공하는 현재 시뮬레이션 시간
4. `first_call = 0`: 다음부터는 초기화 건너뜀
5. `return`: 함수 종료 (첫 실행에서는 계산 안함)

**Python 비교:**
```python
if first_call:
    m_current = P_initial * V_container / (R_gas * T_container)
    previous_time = current_time
    first_call = False
    return
```

## 6. 시간 스텝 계산

```c
real current_time = CURRENT_TIME;
real dt = current_time - previous_time;
previous_time = current_time;
```

**의미:**
- `dt`: 시간 간격 (delta time)
- 이전 시간과 현재 시간의 차이
- 다음 계산을 위해 현재 시간을 저장

## 7. 경계면 찾기

```c
Thread *t = Lookup_Thread(domain, "inlet_r51101_burst");
```

**설명:**
- `Thread*`: 경계면 정보를 담는 포인터
- `Lookup_Thread`: Fluent에서 이름으로 경계면 찾는 함수
- `"inlet_r51101_burst"`: Fluent에서 설정한 경계면 이름

**Python 비교:**
```python
boundary = find_boundary_by_name("inlet_r51101_burst")
```

## 8. 질량 유량 계산 루프

```c
real mass_flow_rate = 0.0;
face_t f;               // 면 ID
cell_t c0, c1;          // 셀 ID들

begin_f_loop(f, t)      // 각 면에 대해 반복
{
    c0 = F_C0(f, t);    // 면의 첫 번째 인접 셀
    c1 = F_C1(f, t);    // 면의 두 번째 인접 셀
    
    if (c1 == -1)       // 외부 경계면이면
    {
        real density = C_R(c0, THREAD_T0(t));     // 셀의 밀도
        mass_flow_rate += density * F_FLUX(f, t);  // 질량 유량 = 밀도 × 부피 유량
    }
}
end_f_loop(f, t)
```

**단계별 설명:**
1. **루프**: 경계면의 모든 작은 면들을 하나씩 처리
2. **인접 셀**: 각 면은 최대 2개 셀과 연결
3. **경계 확인**: `c1 == -1`이면 도메인 외부 (진짜 경계)
4. **밀도 획득**: 인접 셀에서 밀도 값 가져오기
5. **유량 계산**: `F_FLUX`는 부피 유량, 밀도를 곱해서 질량 유량

**Python 비교:**
```python
mass_flow_rate = 0.0
for face in boundary.faces:
    if face.is_external_boundary:
        density = face.adjacent_cell.density
        mass_flow_rate += density * face.volume_flux
```

## 9. 질량 및 압력 업데이트

```c
m_current -= mass_flow_rate * dt;       // 질량 감소
if (m_current < 0.0) m_current = 0.0;   // 물리적 제한

real P_new = (m_current * R_gas * T_container) / V_container;  // 새 압력
P_updated = P_new;                      // 전역 변수 업데이트
```

**물리학:**
1. **질량 보존**: 빠져나간 질량 = 유량 × 시간
2. **음수 방지**: 질량은 0 이하가 될 수 없음
3. **이상기체 법칙**: `P = mRT/V`
4. **전역 저장**: 다른 함수에서 사용할 수 있도록 저장

## 10. 압력 경계 조건 설정 함수

```c
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

**역할:**
- Fluent에서 압력 경계 조건으로 사용
- 모든 면에 동일한 압력값 적용
- `P_updated`: DEFINE_ADJUST에서 계산된 압력 사용

## 11. 전체 작동 원리

1. **초기화**: 용기 내 초기 질량 계산
2. **매 타임 스텝마다**:
   - 출구에서의 질량 유량 계산
   - 용기 내 질량 감소
   - 새로운 압력 계산 (이상기체 법칙)
   - 경계 조건에 새 압력 적용
3. **결과**: 시간에 따른 압력 변화 시뮬레이션

**실제 적용 예:**
- 압력 용기에서 기체 분출
- 타이어 펑크
- 가스통 방출 등

## 12. 사용시 주의사항

1. **경계면 이름**: `"inlet_r51101_burst"`를 실제 모델 이름으로 변경
2. **물성치**: 기체 상수는 사용하는 기체에 맞게 조정
3. **단위**: SI 단위 사용 (Pa, kg, m, s, K)
4. **컴파일**: 직렬 모드에서 컴파일 후 사용
