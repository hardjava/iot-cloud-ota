import { useState } from "react";
import { useLockScreen } from "./LockScreenContext";
import { LockKeyhole } from "lucide-react";

/**
 * LockScreen 컴포넌트
 * 화면을 잠그고 비밀번호로 잠금 해제하는 기능을 제공합니다.
 * NOTE: UI를 먼저 구현한 상태여서, 비밀번호는 환경 변수에서 가져오도록 하였습니다.
 * TODO: 향후 사용자 인증 시스템과 연동하여 비밀번호를 관리하도록 개선이 필요합니다.
 */
const LockScreen = () => {
  const { unlock } = useLockScreen();
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    const correctPassword = import.meta.env.VITE_LOCK_SCREEN_PASSWORD;

    if (password === correctPassword) {
      unlock();
    } else {
      setError("Incorrect password");
      setPassword("");
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50 backdrop-blur-sm">
      <div className="w-full max-w-sm rounded-lg bg-white p-8 shadow-xl dark:bg-gray-800">
        <div className="mx-auto mb-6 flex h-16 w-16 items-center justify-center rounded-full bg-blue-100 dark:bg-blue-900">
          <LockKeyhole className="h-8 w-8 text-blue-500 dark:text-blue-400" />
        </div>
        <h2 className="text-center text-2xl font-bold text-gray-800 dark:text-white">
          화면 잠금
        </h2>
        <p className="mb-6 text-center text-sm text-gray-500 dark:text-gray-400">
          잠금 해제를 위해 비밀번호를 입력하세요.
        </p>
        <form onSubmit={handleSubmit}>
          <input
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            placeholder="비밀번호"
            className="w-full rounded-md border border-gray-300 bg-gray-50 p-3 text-center text-gray-800 focus:border-blue-500 focus:ring-blue-500 dark:border-gray-600 dark:bg-gray-700 dark:text-white dark:placeholder-gray-400"
          />
          {error && (
            <p className="mt-2 text-center text-xs text-red-500">{error}</p>
          )}
          <button
            type="submit"
            className="mt-6 w-full rounded-md bg-blue-600 py-3 font-semibold text-white transition hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 dark:focus:ring-offset-gray-800"
          >
            잠금 해제
          </button>
        </form>
      </div>
    </div>
  );
};

export default LockScreen;
