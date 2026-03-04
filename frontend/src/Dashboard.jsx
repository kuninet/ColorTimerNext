import React, { useMemo } from 'react';
import {
    BarChart, Bar, XAxis, YAxis, Tooltip, ResponsiveContainer, CartesianGrid, Cell
} from 'recharts';
import { format, parseISO, differenceInMinutes, startOfDay, subDays, eachDayOfInterval } from 'date-fns';
import { Clock, Activity } from 'lucide-react';

export default function Dashboard({ logs }) {
    const stats = useMemo(() => {
        if (!logs || logs.length === 0) return { totalMinutes: 0, dailyData: [] };

        const sorted = [...logs].sort((a, b) => new Date(a.Timestamp) - new Date(b.Timestamp));

        // Group by device
        const deviceState = {};
        const sessions = [];

        sorted.forEach(log => {
            const time = new Date(log.Timestamp);
            if (log.Action === 'start') {
                deviceState[log.DeviceId] = time;
            } else if (log.Action === 'stop' && deviceState[log.DeviceId]) {
                const start = deviceState[log.DeviceId];
                const min = differenceInMinutes(time, start);
                sessions.push({ deviceId: log.DeviceId, start, end: time, durationMin: min });
                delete deviceState[log.DeviceId];
            }
        });

        // Add ongoing sessions up to now
        const now = new Date();
        Object.keys(deviceState).forEach(deviceId => {
            const start = deviceState[deviceId];
            const min = differenceInMinutes(now, start);
            sessions.push({ deviceId, start, end: now, durationMin: min });
        });

        let totalMinutes = 0;
        sessions.forEach(s => totalMinutes += s.durationMin);

        // Daily breakdown for the last 7 days
        const last7Days = eachDayOfInterval({
            start: startOfDay(subDays(now, 6)),
            end: startOfDay(now)
        });

        const dailyDataObj = {};
        last7Days.forEach(day => {
            dailyDataObj[format(day, 'yyyy-MM-dd')] = 0;
        });

        sessions.forEach(s => {
            const dayStr = format(s.start, 'yyyy-MM-dd');
            if (dailyDataObj[dayStr] !== undefined) {
                dailyDataObj[dayStr] += s.durationMin;
            }
        });

        const dailyData = Object.keys(dailyDataObj).map(day => ({
            date: format(parseISO(day), 'MM/dd'),
            minutes: dailyDataObj[day]
        }));

        return { totalMinutes, dailyData };
    }, [logs]);

    const formatTime = (totalMin) => {
        const h = Math.floor(totalMin / 60);
        const m = totalMin % 60;
        return `${h}時間 ${m}分`;
    };

    return (
        <div className="space-y-6 animate-in fade-in duration-500">
            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div className="bg-neutral-800/80 backdrop-blur-md rounded-2xl p-6 border border-neutral-700 shadow-xl flex items-center space-x-4 transition-transform hover:scale-[1.02]">
                    <div className="bg-blue-900/50 p-3 rounded-full text-blue-400">
                        <Clock size={32} />
                    </div>
                    <div>
                        <p className="text-neutral-400 text-sm font-medium">累計稼働時間</p>
                        <p className="text-3xl font-bold text-neutral-100">{formatTime(stats.totalMinutes)}</p>
                    </div>
                </div>
                <div className="bg-neutral-800/80 backdrop-blur-md rounded-2xl p-6 border border-neutral-700 shadow-xl flex items-center space-x-4 transition-transform hover:scale-[1.02]">
                    <div className="bg-emerald-900/50 p-3 rounded-full text-emerald-400">
                        <Activity size={32} />
                    </div>
                    <div>
                        <p className="text-neutral-400 text-sm font-medium">記録回数 (Start)</p>
                        <p className="text-3xl font-bold text-neutral-100">{logs.filter(l => l.Action === 'start').length} 回</p>
                    </div>
                </div>
            </div>

            <div className="bg-neutral-800/80 backdrop-blur-md rounded-2xl p-6 border border-neutral-700 shadow-xl">
                <h3 className="text-lg font-semibold text-neutral-200 mb-6 flex items-center gap-2">
                    直近7日間の稼働推移 (分)
                </h3>
                <div className="h-72 w-full">
                    <ResponsiveContainer width="100%" height="100%">
                        <BarChart data={stats.dailyData} margin={{ top: 10, right: 10, left: -20, bottom: 0 }}>
                            <CartesianGrid strokeDasharray="3 3" stroke="#404040" vertical={false} />
                            <XAxis dataKey="date" stroke="#9ca3af" fontSize={12} tickLine={false} axisLine={false} />
                            <YAxis stroke="#9ca3af" fontSize={12} tickLine={false} axisLine={false} />
                            <Tooltip
                                cursor={{ fill: '#262626' }}
                                contentStyle={{ backgroundColor: '#171717', borderColor: '#404040', color: '#f5f5f5', borderRadius: '8px' }}
                                formatter={(value) => [`${value} 分`, '稼働時間']}
                            />
                            <Bar dataKey="minutes" fill="#3b82f6" radius={[4, 4, 0, 0]}>
                                {stats.dailyData.map((entry, index) => (
                                    <Cell key={`cell-${index}`} fill={entry.minutes > 0 ? '#3b82f6' : '#1f2937'} />
                                ))}
                            </Bar>
                        </BarChart>
                    </ResponsiveContainer>
                </div>
            </div>
        </div>
    );
}
