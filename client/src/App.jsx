import React, { useState, useEffect } from 'react';
import axios from 'axios';
import './App.css'; // Vom adauga stiluri imediat

const API_URL = "http://localhost:8080";

function App() {
  // --- Stare (State) ---
  const [step, setStep] = useState('FORM'); // FORM, PAYMENT, SUCCESS, FAIL, EXPIRED
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  
  // Date formular
  const [formData, setFormData] = useState({
    nume: '',
    cnp: '',
    centru_id: 1,
    atelier_id: 1,
    ora: '2023-10-27 10:00:00' // Hardcodat pentru demo, poti pune input type="datetime-local"
  });

  // Date rezervare
  const [rezervareId, setRezervareId] = useState(null);
  
  // Cronometru
  const [timeLeft, setTimeLeft] = useState(12);

  // --- Efect: Cronometrul de 12 secunde ---
  useEffect(() => {
    let timer = null;
    if (step === 'PAYMENT' && timeLeft > 0) {
      timer = setInterval(() => {
        setTimeLeft((prev) => prev - 1);
      }, 1000);
    } else if (timeLeft === 0 && step === 'PAYMENT') {
      setStep('EXPIRED');
    }
    return () => clearInterval(timer);
  }, [step, timeLeft]);

  // --- Handlers ---

  const handleInputChange = (e) => {
    const { name, value } = e.target;
    // Convertim in int pentru ID-uri
    const val = (name === 'centru_id' || name === 'atelier_id') ? parseInt(value) : value;
    setFormData({ ...formData, [name]: val });
  };

  const submitRezervare = async (e) => {
    e.preventDefault();
    setLoading(true);
    setError('');

    try {
      // 1. Trimitem cererea la C++
      const res = await axios.post(`${API_URL}/rezerva`, formData);
      
      if (res.data.status === 'success') {
        setRezervareId(res.data.id);
        setTimeLeft(12); // Resetam timerul
        setStep('PAYMENT');
      } else {
        setError(res.data.message || 'Rezervare eșuată');
        setStep('FAIL');
      }
    } catch (err) {
      setError("Serverul nu răspunde. Verifică dacă este pornit.");
    } finally {
      setLoading(false);
    }
  };

  const submitPlata = async () => {
    setLoading(true);
    try {
      // 2. Trimitem plata
      const res = await axios.post(`${API_URL}/plateste`, {
        id: rezervareId,
        suma: 50.00 // Pret fix pentru demo
      });

      if (res.data.status === 'paid') {
        setStep('SUCCESS');
      } else {
        setError("Plata a eșuat sau timpul a expirat.");
        setStep('FAIL');
      }
    } catch (err) {
      setError("Eroare de conexiune la plată.");
    } finally {
      setLoading(false);
    }
  };

  const resetFlow = () => {
    setStep('FORM');
    setTimeLeft(12);
    setError('');
    setRezervareId(null);
  };

  // --- Interfata (Render) ---
  return (
    <div className="container">
      <div className="card">
        <h1>Ateliere Culturale</h1>
        
        {/* PASUL 1: Formular */}
        {step === 'FORM' && (
          <form onSubmit={submitRezervare}>
            <div className="form-group">
              <label>Nume Client</label>
              <input name="nume" required onChange={handleInputChange} placeholder="Ex: Popescu Ion" />
            </div>
            <div className="form-group">
              <label>CNP</label>
              <input name="cnp" required onChange={handleInputChange} placeholder="123456..." />
            </div>
            <div className="row">
              <div className="form-group">
                <label>Centru</label>
                <select name="centru_id" onChange={handleInputChange}>
                  <option value={1}>Centrul Nord</option>
                  <option value={2}>Centrul Sud</option>
                </select>
              </div>
              <div className="form-group">
                <label>Atelier</label>
                <select name="atelier_id" onChange={handleInputChange}>
                  <option value={1}>Ceramică (50 RON)</option>
                  <option value={2}>Pictură (40 RON)</option>
                </select>
              </div>
            </div>
            <div className="form-group">
              <label>Data și Ora</label>
              <input name="ora" defaultValue={formData.ora} onChange={handleInputChange} />
              <small>Format: YYYY-MM-DD HH:MM:SS</small>
            </div>
            
            <button type="submit" disabled={loading} className="btn-primary">
              {loading ? 'Se verifică...' : 'Verifică Disponibilitate'}
            </button>
            {error && <p className="error">{error}</p>}
          </form>
        )}

        {/* PASUL 2: Plata */}
        {step === 'PAYMENT' && (
          <div className="payment-view">
            <h2>Rezervare Provizorie #{rezervareId}</h2>
            <p>Locul este blocat pentru tine.</p>
            
            <div className="timer-box">
              <h3>Ai {timeLeft} secunde</h3>
              <p>pentru a efectua plata</p>
            </div>

            <button onClick={submitPlata} disabled={loading} className="btn-success">
              {loading ? 'Se procesează...' : 'Plătește 50 RON'}
            </button>
          </div>
        )}

        {/* PASUL 3: Succes */}
        {step === 'SUCCESS' && (
          <div className="result-view success">
            <h2>🎉 Rezervare Confirmată!</h2>
            <p>Te așteptăm la atelier.</p>
            <button onClick={resetFlow}>Rezervare Nouă</button>
          </div>
        )}

        {/* PASUL 4: Esec / Expirat */}
        {(step === 'FAIL' || step === 'EXPIRED') && (
          <div className="result-view fail">
            <h2>{step === 'EXPIRED' ? '⏱️ Timpul a expirat' : '❌ Eroare'}</h2>
            <p>{step === 'EXPIRED' ? 'Nu ai plătit în 12 secunde. Locul a fost eliberat.' : error}</p>
            <button onClick={resetFlow}>Încearcă din nou</button>
          </div>
        )}

      </div>
    </div>
  );
}

export default App;