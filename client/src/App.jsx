import React, { useState, useEffect } from 'react';
import axios from 'axios';
import './App.css'; 

const API_URL = "http://localhost:8080";

function App() {
  const [step, setStep] = useState('FORM');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  
  const [formData, setFormData] = useState({
    nume: '',
    cnp: '',
    centru_id: 1,
    atelier_id: 1,
    ora: new Date().toISOString().slice(0, 16)
  });

  const [rezervareId, setRezervareId] = useState(null);
  const [refundAmount, setRefundAmount] = useState(0);
  const [timeLeft, setTimeLeft] = useState(12);

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

  const handleInputChange = (e) => {
    const { name, value } = e.target;
    const val = (name === 'centru_id' || name === 'atelier_id') ? parseInt(value) : value;
    setFormData({ ...formData, [name]: val });
  };

  const submitRezervare = async (e) => {
    e.preventDefault();

    const dateObj = new Date(formData.ora);
    const ora = dateObj.getHours();

    if (ora < 9 || ora >= 17) {
        alert("⚠️ Centrul este închis!\n\nRezervările se pot face doar între orele 09:00 și 17:00.");
        return;
    }

    setLoading(true);
    setError('');

    const dataPentruServer = {
        ...formData,
        ora: formData.ora.replace('T', ' ') + ':00'
    };

    try {
      const res = await axios.post(`${API_URL}/rezerva`, dataPentruServer);
      
      if (res.data.status === 'success') {
        setRezervareId(res.data.id);
        setTimeLeft(12);
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
      const res = await axios.post(`${API_URL}/plateste`, {
        id: rezervareId,
        suma: 50.00 
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

  const submitAnulare = async () => {
    if (!rezervareId) return;
    setLoading(true);
    try {
        const res = await axios.post(`${API_URL}/anuleaza`, {
            id: rezervareId
        });

        if (res.data.status === 'refunded') {
            setRefundAmount(res.data.amount);
            setStep('CANCELLED');
        } else {
            alert("Eroare la anulare: " + res.data.message);
        }
    } catch (err) {
        console.error(err);
        alert("Eroare de conexiune la server pentru anulare.");
    } finally {
        setLoading(false);
    }
  };

  const resetFlow = () => {
    setStep('FORM');
    setTimeLeft(12);
    setError('');
    setRezervareId(null);
    setRefundAmount(0);
    setFormData(prev => ({...prev, ora: new Date().toISOString().slice(0, 16)}));
  };

  return (
    <div className="container">
      <div className="card">
        <h1>Ateliere Culturale</h1>
        
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
              <input 
                type="datetime-local" 
                name="ora" 
                value={formData.ora} 
                onChange={handleInputChange} 
                required
                min={new Date().toISOString().slice(0, 16)} 
              />
            </div>
            
            <button type="submit" disabled={loading} className="btn-primary">
              {loading ? 'Se verifică...' : 'Verifică Disponibilitate'}
            </button>
            {error && <p className="error">{error}</p>}
          </form>
        )}

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

        {step === 'SUCCESS' && (
          <div className="result-view success">
            <h2>🎉 Rezervare Confirmată!</h2>
            <p>Te așteptăm la atelier.</p>
            
            <div className="action-buttons" style={{ display: 'flex', gap: '10px', justifyContent: 'center', marginTop: '20px' }}>
                <button onClick={resetFlow} className="btn-primary">Rezervare Nouă</button>
                <button 
                    onClick={submitAnulare} 
                    disabled={loading}
                    style={{ backgroundColor: '#dc3545', color: 'white', border: 'none' }}
                >
                    {loading ? 'Se anulează...' : 'Anulează & Cere Refund'}
                </button>
            </div>
          </div>
        )}

        {step === 'CANCELLED' && (
             <div className="result-view fail">
                <h2 style={{color: '#666'}}>Rezervare Anulată</h2>
                <p>Locul a fost eliberat.</p>
                <div style={{ backgroundColor: '#e2e3e5', padding: '10px', borderRadius: '5px', margin: '15px 0', color: '#383d41' }}>
                    <strong>RAMBURS EFECTUAT:</strong> {refundAmount} RON
                </div>
                <button onClick={resetFlow}>Înapoi la început</button>
             </div>
        )}

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