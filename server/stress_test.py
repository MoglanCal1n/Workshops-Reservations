import threading
import time
import requests
import random

NUM_CLIENTI = 20
DURATA_TEST = 120 
INTERVAL_CERERI = 2 
API_URL = "http://localhost:8080"

stats = {
    "req_sent": 0,
    "res_success": 0,
    "res_full": 0,
    "res_closed": 0,
    "paid": 0,
    "errors": 0
}
stats_lock = threading.Lock()

def client_behavior(client_id):
    start_time = time.time()
    
    while time.time() - start_time < DURATA_TEST:

        req_data = {
            "nume": f"Client_{client_id}",
            "cnp": f"100000{client_id}",
            "centru_id": random.randint(1, 3),   
            "atelier_id": random.randint(1, 4),  
            "ora": f"2026-01-31 {random.randint(10, 16)}:00:00" 
        }

        try:
            res = requests.post(f"{API_URL}/rezerva", json=req_data)
            with stats_lock: stats["req_sent"] += 1

            if res.status_code == 200:
                data = res.json()
                if data["status"] == "success":
                    with stats_lock: stats["res_success"] += 1
                    res_id = data["id"]
                    
                    if random.random() < 0.7:
                        time.sleep(random.uniform(1, 10))
                        
                        pay_res = requests.post(f"{API_URL}/plateste", json={"id": res_id, "suma": 50.0})
                        if pay_res.json().get("status") == "paid":
                            with stats_lock: stats["paid"] += 1
                elif "Full" in data.get("message", ""):
                    with stats_lock: stats["res_full"] += 1
                else:
                    with stats_lock: stats["res_closed"] += 1
            else:
                with stats_lock: stats["errors"] += 1

        except Exception as e:
            print(f"Eroare client {client_id}: {e}")
            with stats_lock: stats["errors"] += 1
        
        time.sleep(INTERVAL_CERERI)

def run_test():
    print(f"--- START STRESS TEST ({NUM_CLIENTI} clienti, {DURATA_TEST}s) ---")
    threads = []
    
    for i in range(NUM_CLIENTI):
        t = threading.Thread(target=client_behavior, args=(i,))
        threads.append(t)
        t.start()
        time.sleep(0.1) 

    for t in threads:
        t.join()

    print("\n--- TEST COMPLET ---")
    print(f"Total Cereri Trimise: {stats['req_sent']}")
    print(f"Rezervari Reusite:    {stats['res_success']}")
    print(f"Rezervari Full:       {stats['res_full']}")
    print(f"Plati Efectuate:      {stats['paid']}")
    print(f"Erori HTTP:           {stats['errors']}")

if __name__ == "__main__":
    run_test()