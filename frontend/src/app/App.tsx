import { RouterProvider } from "react-router";
import { Router } from "./Router";
import { css, Global } from "@emotion/react";
import { ToastContainer } from "react-toastify";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { LockScreenProvider, useLockScreen } from "../shared/ui/LockScreenContext";
import LockScreen from "../shared/ui/LockScreen";

const queryClient = new QueryClient();

function AppContent() {
  const { isLocked } = useLockScreen();
  return (
    <>
      <Global
        styles={css`
          * {
            font-family: "Pretendard";
          }
        `}
      />
      <RouterProvider router={Router} />
      <ToastContainer />
      {isLocked && <LockScreen />}
    </>
  );
}

function App() {
  return (
    <QueryClientProvider client={queryClient}>
      <LockScreenProvider>
        <AppContent />
      </LockScreenProvider>
    </QueryClientProvider>
  );
}

export default App;
