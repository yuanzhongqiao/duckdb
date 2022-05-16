//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/parallel/pipeline.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/unordered_set.hpp"
#include "duckdb/execution/physical_operator.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/parallel/parallel_state.hpp"
#include "duckdb/parallel/task_scheduler.hpp"
#include "duckdb/common/atomic.hpp"

namespace duckdb {
class Executor;
class Event;

class PipelineBuildState {
public:
	//! The current recursive CTE node (if any)
	PhysicalOperator *recursive_cte = nullptr;

	//! Duplicate eliminated join scan dependencies
	unordered_map<PhysicalOperator *, Pipeline *> delim_join_dependencies;

public:
	void SetPipelineSource(Pipeline &pipeline, PhysicalOperator *op);
	void SetPipelineSink(Pipeline &pipeline, PhysicalOperator *op);
	void SetPipelineOperators(Pipeline &pipeline, vector<PhysicalOperator *> operators);
	void AddPipelineOperator(Pipeline &pipeline, PhysicalOperator *op);
	void AddPipeline(Executor &executor, shared_ptr<Pipeline> pipeline);
	void AddChildPipeline(Executor &executor, Pipeline &pipeline);

	unordered_map<Pipeline *, vector<shared_ptr<Pipeline>>> &GetUnionPipelines(Executor &executor);
	unordered_map<Pipeline *, vector<shared_ptr<Pipeline>>> &GetChildPipelines(Executor &executor);
	unordered_map<Pipeline *, vector<Pipeline *>> &GetChildDependencies(Executor &executor);

	PhysicalOperator *GetPipelineSink(Pipeline &pipeline);
	vector<PhysicalOperator *> GetPipelineOperators(Pipeline &pipeline);
};

//! The Pipeline class represents an execution pipeline
class Pipeline : public std::enable_shared_from_this<Pipeline> {
	friend class Executor;
	friend class PipelineExecutor;
	friend class PipelineEvent;
	friend class PipelineFinishEvent;
	friend class PipelineBuildState;

public:
	explicit Pipeline(Executor &execution_context);

	Executor &executor;

public:
	ClientContext &GetClientContext();

	void AddDependency(shared_ptr<Pipeline> &pipeline);

	void Ready();
	void Reset();
	void ResetSource();
	void Schedule(shared_ptr<Event> &event);

	//! Finalize this pipeline
	void Finalize(Event &event);

	string ToString() const;
	void Print() const;

	//! Returns query progress
	bool GetProgress(double &current_percentage);

	//! Returns a list of all operators (including source and sink) involved in this pipeline
	vector<PhysicalOperator *> GetOperators() const;

	PhysicalOperator *GetSink() {
		return sink;
	}

private:
	//! Whether or not the pipeline has been readied
	bool ready;
	//! The source of this pipeline
	PhysicalOperator *source;
	//! The chain of intermediate operators
	vector<PhysicalOperator *> operators;
	//! The sink (i.e. destination) for data; this is e.g. a hash table to-be-built
	PhysicalOperator *sink;

	//! The global source state
	unique_ptr<GlobalSourceState> source_state;

	//! The parent pipelines (i.e. pipelines that are dependent on this pipeline to finish)
	vector<weak_ptr<Pipeline>> parents;
	//! The dependencies of this pipeline
	vector<weak_ptr<Pipeline>> dependencies;

private:
	bool GetProgressInternal(ClientContext &context, PhysicalOperator *op, double &current_percentage);
	void ScheduleSequentialTask(shared_ptr<Event> &event);
	bool LaunchScanTasks(shared_ptr<Event> &event, idx_t max_threads);

	bool ScheduleParallel(shared_ptr<Event> &event);
};

} // namespace duckdb
