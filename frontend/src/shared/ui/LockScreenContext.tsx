import { createContext, useContext, useState, ReactNode } from 'react';

/**
 * LockScreenContext의 타입 정의
 * 화면 잠금 상태와 잠금/잠금 해제 함수를 포함합니다.
 * - isLocked: 화면이 잠금 상태인지 여부
 *  - lock: 화면을 잠금 상태로 설정하는 함수
 *  - unlock: 화면 잠금을 해제하는 함수
 */
interface LockScreenContextType {
  isLocked: boolean;
  lock: () => void;
  unlock: () => void;
}

/**
 * LockScreenContext 생성
 */
const LockScreenContext = createContext<LockScreenContextType | undefined>(
  undefined,
);

/**
 * LockScreenProvider 컴포넌트
 * 자식 컴포넌트들에게 LockScreenContext를 제공합니다.
 */
export const LockScreenProvider = ({ children }: { children: ReactNode }) => {
  const [isLocked, setIsLocked] = useState(() => {
    return localStorage.getItem('isLocked') === 'true';
  });

  const lock = () => {
    localStorage.setItem('isLocked', 'true');
    setIsLocked(true);
  };

  const unlock = () => {
    localStorage.removeItem('isLocked');
    setIsLocked(false);
  };

  return (
    <LockScreenContext.Provider value={{ isLocked, lock, unlock }}>
      {children}
    </LockScreenContext.Provider>
  );
};

/**
 * LockScreenContext를 사용하는 커스텀 훅
 * LockScreenContext가 제공되지 않는 곳에서 사용될 경우 에러를 발생시킵니다.
 */
export const useLockScreen = () => {
  const context = useContext(LockScreenContext);
  if (context === undefined) {
    throw new Error("useLockScreen must be used within a LockScreenProvider");
  }
  return context;
};
