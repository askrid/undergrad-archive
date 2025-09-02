import { useNavigate } from "react-router-dom";

const GoToArticleCreateButton = () => {
  const navigate = useNavigate();

  const handleClick = () => {
    navigate("/articles/create");
  };

  return (
    <button
      id="create-article-button"
      onClick={() => handleClick()}
      className="font-semibold text-white px-4 py-2 bg-slate-800 rounded"
    >
      <span className="sm:inline-block hidden">Create a New Article</span>
      <span className="sm:hidden inline-block">New Article</span>
    </button>
  );
};

export default GoToArticleCreateButton;
