import { useState, useEffect } from 'react';
import axios from 'axios';
import { format } from 'date-fns';

function App() {
  const [logs, setLogs] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  useEffect(() => {
    // API Gatewayのエンドポイント (環境変数から取得するか、Vite起動時にプロキシする想定)
    // ここでは開発用にダミーデータを一旦モックするか、実際のデプロイ先URLを入れる
    // 実際の実装時にviteのエンドポイント差し替えを行う
    const API_URL = import.meta.env.VITE_API_URL || '/api/logs';
    const API_KEY = import.meta.env.VITE_API_KEY || '';

    const fetchLogs = async () => {
      try {
        setLoading(true);
        const response = await axios.get(API_URL, {
          headers: {
            'x-api-key': API_KEY
          }
        });

        // 取得したデータを新しい順(降順)にならべるなどの処理
        // API側ですでに降順になっている場合はそのまま
        const sortedLogs = response.data.items.sort((a, b) => new Date(b.Timestamp) - new Date(a.Timestamp));
        setLogs(sortedLogs);
        setError(null);
      } catch (err) {
        console.error("API Fetch Error:", err);
        setError("履歴データの取得に失敗しました。");

        // 開発用のダミーデータ (APIに繋がらない時のフォールバック)
        if (import.meta.env.DEV) {
          setLogs([
            { DeviceId: 'esp32-1234', Timestamp: new Date().toISOString(), Action: 'start' },
            { DeviceId: 'esp32-1234', Timestamp: new Date(Date.now() - 3600000).toISOString(), Action: 'stop' },
            { DeviceId: 'esp32-1234', Timestamp: new Date(Date.now() - 7200000).toISOString(), Action: 'start' },
          ]);
        }
      } finally {
        setLoading(false);
      }
    };

    fetchLogs();
  }, []);

  return (
    <div className="min-h-screen bg-neutral-900 text-neutral-100 font-sans p-6 sm:p-10">
      <div className="max-w-4xl mx-auto">
        <header className="mb-10 text-center sm:text-left">
          <h1 className="text-3xl sm:text-4xl font-black tracking-tight text-transparent bg-clip-text bg-gradient-to-r from-blue-400 to-indigo-500">
            ColorTimer Next
          </h1>
          <p className="text-neutral-400 mt-2 text-sm sm:text-base">
            デバイス作業履歴一覧
          </p>
        </header>

        {error && !import.meta.env.DEV && (
          <div className="bg-red-900/50 border border-red-500 text-red-200 px-4 py-3 rounded mb-6 backdrop-blur-sm">
            {error}
          </div>
        )}

        {loading ? (
          <div className="flex justify-center items-center h-40">
            <div className="animate-spin rounded-full h-10 w-10 border-t-2 border-b-2 border-blue-500"></div>
          </div>
        ) : (
          <div className="bg-neutral-800/80 backdrop-blur-md rounded-2xl shadow-xl border border-neutral-700 overflow-hidden">
            <div className="overflow-x-auto">
              <table className="w-full text-left border-collapse">
                <thead>
                  <tr className="bg-neutral-800/50 border-b border-neutral-700 text-neutral-300 text-xs uppercase tracking-wider">
                    <th className="p-4 font-semibold">日時 (Timestamp)</th>
                    <th className="p-4 font-semibold">アクション (Action)</th>
                    <th className="p-4 font-semibold">デバイスID (Device)</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-neutral-700/50">
                  {logs.map((log, index) => (
                    <tr
                      key={`${log.DeviceId}-${log.Timestamp}-${index}`}
                      className="hover:bg-neutral-700/30 transition-colors duration-150"
                    >
                      <td className="p-4 whitespace-nowrap text-sm text-neutral-200 font-mono">
                        {format(new Date(log.Timestamp), 'yyyy/MM/dd HH:mm:ss')}
                      </td>
                      <td className="p-4 whitespace-nowrap">
                        <span className={`inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium border ${log.Action === 'start'
                            ? 'bg-blue-900/30 text-blue-400 border-blue-800'
                            : 'bg-rose-900/30 text-rose-400 border-rose-800'
                          }`}>
                          {log.Action === 'start' ? '▶ Start' : '■ Stop'}
                        </span>
                      </td>
                      <td className="p-4 whitespace-nowrap text-sm text-neutral-400">
                        {log.DeviceId}
                      </td>
                    </tr>
                  ))}

                  {logs.length === 0 && (
                    <tr>
                      <td colSpan="3" className="p-8 text-center text-neutral-500">
                        履歴データがありません
                      </td>
                    </tr>
                  )}
                </tbody>
              </table>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}

export default App;
