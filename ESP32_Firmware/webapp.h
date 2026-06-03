#pragma once

const char HTML_CONTENT[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Knee Monitor</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800;900&display=swap');
        body { font-family: 'Inter', sans-serif; }
        @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: .5; } }
        .animate-pulse { animation: pulse 2s cubic-bezier(0.4, 0, 0.6, 1) infinite; }
    </style>
</head>
<body class="bg-gray-100">
    <div id="root"></div>

    <script src="https://unpkg.com/react@18/umd/react.production.min.js"></script>
    <script src="https://unpkg.com/react-dom@18/umd/react-dom.production.min.js"></script>
    <script src="https://unpkg.com/@babel/standalone/babel.min.js"></script>

    <script type="text/babel">
    window.addEventListener('load', () => {
      const { useState, useEffect, useRef, useCallback } = React;

      function App() {
        const [liveData, setLiveData] = useState({ angle: 0, vibration_mv: 0 });
        const [isPolling, setIsPolling] = useState(false);
        const [appMode, setAppMode] = useState('idle');
        const [statusMessage, setStatusMessage] = useState('Ready to start.');
        
        const [baselineData, setBaselineData] = useState([]);
        const [movementData, setMovementData] = useState([]);
        const [calibrationTimer, setCalibrationTimer] = useState(30);
        const [calibrationPhase, setCalibrationPhase] = useState(1);
        const [thresholds, setThresholds] = useState(null);
        const [angleOffset, setAngleOffset] = useState(0);
        const [alertReason, setAlertReason] = useState('');
        
        const pollIntervalRef = useRef(null);
        const calibrationTimerRef = useRef(null);

        const getMean = (arr) => arr.reduce((a, b) => a + b, 0) / arr.length;
        const getStdDev = (arr, mean) => {
          const squareDiffs = arr.map(val => Math.pow(val - mean, 2));
          return Math.sqrt(getMean(squareDiffs));
        };

        const triggerVibration = () => {
          if ('vibrate' in navigator) {
            navigator.vibrate([500, 100, 500]);
          }
        };

        const calculateThresholds = useCallback(() => {
          // FIX #3: Allow calculation even with minimal data
          if (baselineData.length === 0 && movementData.length === 0) {
            setStatusMessage('Calibration failed: No data collected.');
            setAppMode('idle');
            return;
          }
          
          // If stopped early, use whatever data we have
          const calibData = movementData.length > 0 ? movementData : baselineData;
          
          const baselineAngles = baselineData.length > 0 
            ? baselineData.map(d => parseFloat(d.angle))
            : calibData.map(d => parseFloat(d.angle));
          const baseline = getMean(baselineAngles);
          setAngleOffset(baseline);
          
          const movementAngles = calibData.map(d => parseFloat(d.angle));
          const movementVibrations = calibData.map(d => parseFloat(d.vibration_mv));
          
          const relativeAngles = movementAngles.map(a => a - baseline);
          
          const angleMin = Math.min(...relativeAngles);
          const angleMax = Math.max(...relativeAngles);
          const vibrationMean = getMean(movementVibrations);
          const vibrationStd = getStdDev(movementVibrations, vibrationMean);
          
          const newThresholds = {
            angleMin,
            angleMax,
            angleSafeMax: angleMax + 10,
            vibrationMean,
            vibrationStd,
            vibrationSafe: vibrationMean + 2 * vibrationStd
          };
          
          setThresholds(newThresholds);
          setStatusMessage('Calibration complete! Baseline and thresholds set.');
          setAppMode('idle');
        }, [baselineData, movementData]);

        // FIX #1 & #2: Timer countdown logic
        useEffect(() => {
          if (appMode === 'calibration') {
            calibrationTimerRef.current = setInterval(() => {
              setCalibrationTimer((prev) => {
                const newTime = prev - 1;
                
                // FIX #2: Transition at 20s (after 10s of phase 1)
                if (newTime === 20) {
                  setCalibrationPhase(2);
                  setStatusMessage('Phase 2: Perform full range of motion');
                }
                
                if (newTime <= 0) {
                  clearInterval(calibrationTimerRef.current);
                  calculateThresholds();
                  return 30; // Reset for next time
                }
                
                return newTime;
              });
            }, 1000);
            
            return () => clearInterval(calibrationTimerRef.current);
          }
        }, [appMode, calculateThresholds]);

        const checkThresholds = useCallback((relativeAngle, vibrationVal) => {
          if (!thresholds || appMode !== 'monitoring') return;
          
          let alertMsg = '';
          if (relativeAngle > thresholds.angleSafeMax) {
            alertMsg = `Knee angle exceeded! (${relativeAngle.toFixed(1)}° > ${thresholds.angleSafeMax.toFixed(1)}°)`;
          } else if (vibrationVal > thresholds.vibrationSafe) {
            alertMsg = `Vibration exceeded! (${vibrationVal.toFixed(1)} mV > ${thresholds.vibrationSafe.toFixed(1)} mV)`;
          }
          
          if (alertMsg) {
            setAlertReason(alertMsg);
            setAppMode('alert');
            setStatusMessage('ALERT! Limit exceeded.');
            triggerVibration();
          }
        }, [thresholds, appMode]);

        const fetchData = useCallback(async () => {
          try {
            const response = await fetch('/data');
            if (!response.ok) throw new Error('Failed to fetch');
            const data = await response.json();
            
            const rawAngle = parseFloat(data.angle);
            const relativeAngle = rawAngle - angleOffset;
            const displayData = { 
              angle: relativeAngle.toFixed(1), 
              vibration_mv: parseFloat(data.vibration_mv).toFixed(1)
            };
            
            setLiveData(displayData);
            
            if (appMode === 'calibration') {
              if (calibrationPhase === 1) {
                setBaselineData(prev => [...prev, data]);
              } else {
                setMovementData(prev => [...prev, data]);
              }
            } else if (appMode === 'monitoring') {
              setStatusMessage('Monitoring...');
              checkThresholds(relativeAngle, parseFloat(data.vibration_mv));
            }
          } catch (error) {
            console.error('Fetch error:', error);
            setStatusMessage('Connection lost. Retrying...');
          }
        }, [appMode, calibrationPhase, angleOffset, checkThresholds]);

        useEffect(() => {
          if (isPolling) {
            fetchData();
            pollIntervalRef.current = setInterval(fetchData, 300);
          } else {
            clearInterval(pollIntervalRef.current);
          }
          return () => clearInterval(pollIntervalRef.current);
        }, [isPolling, fetchData]);

        const handleStartPolling = () => {
          setIsPolling(true);
          setStatusMessage('Connecting...');
        };

        const handleStopPolling = () => {
          setIsPolling(false);
          setAppMode('idle');
          setStatusMessage('Stopped.');
        };

        const handleStartCalibration = () => {
          if (!isPolling) {
            setStatusMessage('Start polling first!');
            return;
          }
          setBaselineData([]);
          setMovementData([]);
          setCalibrationTimer(30);
          setCalibrationPhase(1);
          setAngleOffset(0);
          setAppMode('calibration');
          setStatusMessage('Phase 1: Stay in initial position');
        };

        const handleStartMonitoring = () => {
          if (!isPolling) {
            setStatusMessage('Start polling first!');
            return;
          }
          if (!thresholds) {
            setStatusMessage('Run calibration first!');
            return;
          }
          setAppMode('monitoring');
          setStatusMessage('Monitoring...');
        };

        // FIX #3: Stop calibration and calculate thresholds
        const handleStop = () => {
          if (appMode === 'calibration') {
            clearInterval(calibrationTimerRef.current);
            // Calculate thresholds with whatever data we have
            calculateThresholds();
            setCalibrationTimer(30); // Reset timer
            setCalibrationPhase(1);  // Reset phase
          } else {
            setAppMode('idle');
          }
        };

        const dismissAlert = () => {
          setAppMode('monitoring');
          setStatusMessage('Alert dismissed. Resuming...');
          setAlertReason('');
        };

        return (
          <div className="min-h-screen p-6 pb-20">
            {appMode === 'alert' && (
              <div className="fixed inset-0 bg-black bg-opacity-75 flex items-center justify-center z-50 p-4">
                <div className="bg-white max-w-lg w-full rounded-2xl shadow-2xl overflow-hidden animate-pulse">
                  <div className="p-8 text-center">
                    <div className="text-6xl mb-4">⚠</div>
                    <h2 className="text-3xl font-extrabold text-red-700 mb-4">ALERT!</h2>
                    <p className="text-lg text-gray-800 font-medium mb-8">{alertReason}</p>
                    <button
                      onClick={dismissAlert}
                      className="w-full px-6 py-3 bg-red-600 text-white text-lg font-semibold rounded-lg shadow-lg hover:bg-red-700"
                    >
                      Dismiss
                    </button>
                  </div>
                </div>
              </div>
            )}

            <header className="bg-white shadow-md rounded-lg p-6 mb-6">
              <h1 className="text-3xl font-bold text-blue-800">Knee Rehabilitation Monitor</h1>
            </header>

            <main className="space-y-6">
              <div className="bg-white p-6 rounded-xl shadow-lg">
                <h2 className="text-xl font-semibold mb-4">Device Control</h2>
                <div className="flex gap-2">
                  {!isPolling ? (
                    <button onClick={handleStartPolling} className="px-6 py-3 bg-blue-600 text-white font-semibold rounded-lg shadow-md hover:bg-blue-700">
                      Start Polling
                    </button>
                  ) : (
                    <button onClick={handleStopPolling} className="px-6 py-3 bg-red-600 text-white font-semibold rounded-lg shadow-md hover:bg-red-700">
                      Stop Polling
                    </button>
                  )}
                </div>
                <p className="text-sm text-gray-500 mt-3">{statusMessage}</p>
              </div>

              <div className="bg-white p-6 rounded-xl shadow-lg">
                <h2 className="text-xl font-semibold mb-4">Two-Phase Calibration</h2>
                {appMode === 'calibration' ? (
                  <div className="text-center">
                    <div className="text-5xl font-bold text-yellow-600 mb-3">{calibrationTimer}s</div>
                    <div className="mb-4">
                      <span className={`inline-block px-4 py-2 rounded-full font-semibold ${
                        calibrationPhase === 1 ? 'bg-blue-100 text-blue-800' : 'bg-green-100 text-green-800'
                      }`}>
                        {calibrationPhase === 1 ? 'Phase 1: Stay Still' : 'Phase 2: Move Knee'}
                      </span>
                    </div>
                    <p className="text-gray-600 mb-4">
                      {calibrationPhase === 1 
                        ? 'Remain in initial position to set baseline' 
                        : 'Perform full range of motion movements'}
                    </p>
                    <button onClick={handleStop} className="px-4 py-2 bg-gray-600 text-white font-semibold rounded-lg hover:bg-gray-700">
                      Stop Calibration
                    </button>
                  </div>
                ) : (
                  <div>
                    <p className="text-gray-600 mb-2">30-second calibration:</p>
                    <ul className="text-sm text-gray-600 mb-4 space-y-1">
                      <li>• <strong>0-10s:</strong> Stay in initial position (baseline)</li>
                      <li>• <strong>10-30s:</strong> Perform full range of motion</li>
                    </ul>
                    <button onClick={handleStartCalibration} disabled={!isPolling || appMode === 'monitoring'} 
                      className="px-4 py-2 bg-yellow-500 text-white font-semibold rounded-lg hover:bg-yellow-600 disabled:bg-gray-300">
                      Start Calibration
                    </button>
                  </div>
                )}
                {thresholds && appMode !== 'calibration' && (
                  <div className="mt-6 p-4 bg-gray-50 rounded-lg border">
                    <h3 className="font-semibold text-gray-700 mb-2">Calibration Results</h3>
                    <div className="text-sm space-y-1">
                      <div>Baseline Angle: {angleOffset.toFixed(1)}° (zeroed to 0°)</div>
                      <div>ROM Range: {thresholds.angleMin.toFixed(1)}° to {thresholds.angleMax.toFixed(1)}°</div>
                      <div className="text-red-600 font-semibold">Alert Threshold: {thresholds.angleSafeMax.toFixed(1)}°</div>
                      <div>Vibration Mean: {thresholds.vibrationMean.toFixed(1)} mV</div>
                      <div className="text-red-600 font-semibold">Vibration Alert: {thresholds.vibrationSafe.toFixed(1)} mV</div>
                    </div>
                  </div>
                )}
              </div>

              <div className="bg-white p-6 rounded-xl shadow-lg">
                <div className="flex justify-between items-center mb-4">
                  <h2 className="text-xl font-semibold">Real-Time Data</h2>
                  {appMode !== 'monitoring' ? (
                    <button onClick={handleStartMonitoring} disabled={!isPolling || appMode === 'calibration' || !thresholds}
                      className="px-4 py-2 bg-green-600 text-white font-semibold rounded-lg hover:bg-green-700 disabled:bg-gray-300">
                      Start Monitoring
                    </button>
                  ) : (
                    <button onClick={handleStop} className="px-4 py-2 bg-gray-600 text-white font-semibold rounded-lg hover:bg-gray-700">
                      Stop Monitoring
                    </button>
                  )}
                </div>
                <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
                  <div className="bg-blue-50 p-6 rounded-lg text-center">
                    <div className="text-sm font-medium text-blue-800 uppercase mb-1">Knee Angle (Relative)</div>
                    <div className="text-5xl font-bold text-blue-600">
                      {liveData.angle}<span className="text-3xl">°</span>
                    </div>
                  </div>
                  <div className="bg-purple-50 p-6 rounded-lg text-center">
                    <div className="text-sm font-medium text-purple-800 uppercase mb-1">Patellar Vibration</div>
                    <div className="text-5xl font-bold text-purple-600">
                      {liveData.vibration_mv}<span className="text-3xl">mV</span>
                    </div>
                    <div className="text-xs text-gray-600 mt-2">
                      {!thresholds ? 'Not calibrated' :
                       parseFloat(liveData.vibration_mv) < thresholds.vibrationMean ? '✓ Low' :
                       parseFloat(liveData.vibration_mv) < thresholds.vibrationSafe ? '⚠ Moderate' : '🔴 High'}
                    </div>
                  </div>
                </div>
              </div>
            </main>

            <footer className={"fixed bottom-0 left-0 right-0 p-3 shadow-inner " + 
              (appMode === 'alert' ? "bg-red-600 text-white" : 
               appMode === 'calibration' ? "bg-yellow-500 text-white" :
               isPolling ? "bg-green-600 text-white" : "bg-gray-700 text-gray-200")}>
              <div className="max-w-4xl mx-auto text-center font-medium">{statusMessage}</div>
            </footer>
          </div>
        );
      }

      const root = ReactDOM.createRoot(document.getElementById('root'));
      root.render(<App />);
    });
    </script>
</body>
</html>
)rawliteral";
