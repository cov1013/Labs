# cov1013core

## 실행 방법
1. 각 서버 빌드
2. `Configs` 폴더를 `Binaries` 내부의 각 실행 파일이 존재하는 곳으로 복사한다.
3. 서버 실행.

## DBConnector 에러 
- SSL Error 발생 시 연결하고자 하는 MySQL-Server가 지원하는 `libmysql.dll`을 사용해야한다.
- 8.x 버전의 경우 `lib.dll`, `lib.dll`을 `\Binaries\*.exe` 폴더에 넣어야한다.