import { unwrapResult } from "@reduxjs/toolkit";
import { useDispatch } from "react-redux";
import { AppDispatch } from "../../store";
import { logout } from "../../store/slices/auth";

const LogoutButton = () => {
  const dispatch = useDispatch<AppDispatch>();

  const handleClick = async (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => {
    try {
      unwrapResult(await dispatch(logout()));
    } catch (e) {
      console.log(`Logout failed: ${e}`);
    }
  };

  return (
    <button
      id="logout-button"
      type="button"
      onClick={(e) => handleClick(e)}
      className="font-semibold px-4 py-2 rounded border-2 border-slate-800"
    >
      Log Out
    </button>
  );
};

export default LogoutButton;
