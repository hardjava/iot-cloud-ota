import Divider from "@mui/material/Divider";
import { NavigationMenu } from "./NavigationMenu";
import { useMediaQuery } from "@mui/material";
import { Lock } from "lucide-react";
import { useLockScreen } from "../../../shared/ui/LockScreenContext";

export const SideBar = () => {
  const isMobile = useMediaQuery("(max-width: 1024px)"); // Hook for mobile view
  const { lock } = useLockScreen();

  if (isMobile) {
    return null; // Hide sidebar on mobile
  }

  return (
    <div className="flex flex-col h-full bg-slate-900 w-60 fixed left-0 top-0">
      <div className="flex flex-col flex-1">
        <div className="flex flex-col items-center justify-center mt-8">
          <div className="flex items-center justify-center w-32 h-32 rounded-full">
            <img
              src="/person.png"
              alt="Profile"
              className="w-full h-full rounded-full object-cover"
            />
          </div>
          {/* TODO: Get Username */}
          <p className="mt-4 text-gray-300 ">Junwoo Park</p>
        </div>
        <Divider
          sx={{
            bgcolor: "#505050",
            width: "66%",
            margin: "0 auto",
            my: 6,
          }}
        />
        <NavigationMenu />
      </div>
      <div className="p-4">
        <button
          onClick={lock}
          className="w-full flex items-center justify-center gap-2 rounded-md px-3 py-2 text-sm font-medium text-gray-300 hover:bg-slate-700 hover:text-white transition-colors"
        >
          <Lock className="h-4 w-4" />
          <span>Lock Screen</span>
        </button>
      </div>
    </div>
  );
};
