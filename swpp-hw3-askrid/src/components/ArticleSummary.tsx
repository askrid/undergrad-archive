import { useNavigate } from "react-router-dom";

interface Props {
  id: number;
  title: string;
  authorName: string;
}

const ArticleSummary = ({ id, title, authorName }: Props) => {
  const navigate = useNavigate();

  const handleTitleClick = () => {
    navigate(`/articles/${id}`);
  };

  return (
    <article className="p-3 h-full bg-white shadow-lg hover:shadow-xl rounded hover:brightness-95">
      <div className="flex justify-between font-semibold pb-2 mb-3 border-b">
        <div className="text-slate-700">{authorName}</div>
        <div className="text-white bg-slate-700 rounded px-1">{id}</div>
      </div>
      <h2>
        {/* Satisfying specification */}
        <button onClick={() => handleTitleClick()} className="text-justify">
          {title}
        </button>
      </h2>
    </article>
  );
};

export default ArticleSummary;
