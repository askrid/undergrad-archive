import { useNavigate } from "react-router-dom";

interface Props {
  elementId: "back-detail-article-button" | "back-create-article-button";
}

const BackToArticlesButton = ({ elementId }: Props) => {
  const navigate = useNavigate();

  const handleClick = () => {
    navigate("/articles");
  };

  return (
    <button
      id={elementId}
      onClick={() => handleClick()}
      className="font-semibold text-white px-4 py-2 rounded bg-slate-800"
    >
      Back
    </button>
  );
};

export default BackToArticlesButton;
