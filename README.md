# Sistem de Rezervari Ateliere Culturare (Client-Server Concurent)

Acest proiect implementeaza o aplicatie client-server pentru gestionarea rezervarilor la ateliere culturale, folosind programare concurenta in C++.

## Tehnologii Utilizate:
- **Backend:** C++, Crow pentru HTTP
- **Frontend:** React + Vite
- **Concurenta:** Thread Pool custom, std::future/promise, Mutex-uri pentru sincronizare
- **Baza de date:** SQLite3 (Mod WAL - Write-Ahead Logging).

## Compilare si instalare

### 1. Cerinte
- Compilator C++ (g++ cu support C++17/20)
- Biblioteci: 'sqlite3', 'ws2_32', 'mswsock'
- Python 3 folosit pentru testarea aplicatiei
- Node.js pentru frontend

### 2. Compilarea Serverului
- Deschideti terminalul, 'cd server' -> 'g++ -O3 main.cpp database.cpp service.cpp -o server.exe -lsqlite3 -DCROW_CAN_USE_ASIO -lws2_32 -lmswsock'

### 3. Pregatirea Bazei de date
- rm workshops.db
- ./server.exe 8 5 15 si dupa Ctrl+C (Pornim scurt serverul pentru a crea tabelele goale si apoi oprim)
- sqlite3 workshops.db ".read setup_data.sql" 

### 4. Pornim aplicatia
- ./server.exe 8 5 15 -> aplicatia accepta urmatorii parametrii de la linia de comanda 
    - ./server.exe <nr_threaduri> <interval_audit> <timeout_plata>

- Pentru Client
    - cd client
    - npm install
    - npm run dev

### 5. Testare aplicatie
- Dupa pornirea serverului deschidem un alt terminal din care rulam: python stress_test.py