import { useSelector } from "react-redux";
import { useNavigate } from "react-router-dom";
import { selectArticle } from "../../store/slices/article";

const BackToArticleButton = () => {
  const navigate = useNavigate();
  const { selectedArticle, writingArticle } = useSelector(selectArticle);

  const handleClick = () => {
    if (
      selectedArticle?.title === writingArticle.title &&
      selectedArticle?.content === writingArticle.content
    ) {
      navigate(`/articles/${selectedArticle.id}`);
    } else {
      if (window.confirm("Are you sure? The change will be lost.")) {
        navigate(`/articles/${selectedArticle?.id}`);
      }
    }
  };

  return (
    <button
      id="back-edit-article-button"
      onClick={() => handleClick()}
      className="font-semibold text-white px-4 py-2 rounded bg-slate-800"
    >
      Back
    </button>
  );
};

export default BackToArticleButton;
