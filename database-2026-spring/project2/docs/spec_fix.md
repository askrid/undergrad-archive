1. recommend_user_based 출력 컬럼

스펙 실행예시 헤더는 'id title director age_limit avg.rating exp.rating' 6컬럼인데 데이터 행에서 5개인 부분은 오타이므로 6개로 구현해주시기 바랍니다.
 

2. 회원 삽입 성공 메시지 표기

스펙 실행 예시에는 'User successfully inserted'로 나오는데 2026-1_project2-msg.pdf S2에는 'One user successfully inserted'로 되어 있습니다.
스펙 실행 예시는 오타이므로 2026-1_project2-msg.pdf를 따라주시기 바랍니다.
 

3. 메뉴 13(search directors) 동작

run.py 스켈레톤에 메뉴 13 'search directors'가 있는데 해당 부분은 스펙에 없으므로 구현을 생략해주시기 바랍니다.
 

4. 같은 회원이 같은 DVD를 중복 대출 시도

스펙에 명시된 케이스가 아니므로 별도 에러 처리를 요구하지 않고 TC에도 해당케이스는 없도록 하겠습니다.
이상입니다.
 

5. 예약 취소 성공 메시지 (spec 2-10)

성공 메시지는 다음과 같이 구현하시면 됩니다:
Reservation for DVD {d_id} successfully cancelled
{d_id} 자리에 취소된 DVD의 ID를 출력합니다.

