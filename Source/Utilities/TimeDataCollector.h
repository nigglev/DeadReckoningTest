#pragma once
#include <list>
#include <algorithm>
#include "CoreMinimal.h"

template<typename TimeType, typename DataType, typename ChangingType>
class FTimeChangeCollector
{
private:
	struct SElem
	{
		TimeType Stamp_;
		DataType Value_;
		TimeType Duration_;
		ChangingType Changed_;
	};
public:

	using TCalcChangeFunc = TFunction<ChangingType(const DataType&, const DataType&)>;

	FTimeChangeCollector() = default;
	FTimeChangeCollector(TimeType InMaxTime): MaxTime_(InMaxTime) {}

	FTimeChangeCollector(const FTimeChangeCollector& Other)
	: MaxTime_(Other.MaxTime_), Collection_(Other.Collection_), FullDuration_(Other.FullDuration_), SumChanges_(Other.SumChanges_) {}

	FTimeChangeCollector(FTimeChangeCollector&& Other) noexcept
		: MaxTime_(Other.MaxTime_), Collection_(std::move(Other.Collection_)), FullDuration_(Other.FullDuration_), SumChanges_(Other.SumChanges_)
	{		
	}

	void Add(TimeType InTimeStamp, DataType InValue, const TCalcChangeFunc& InCalcChangeFunc)
	{
		if(Collection_.empty())
		{
			Collection_.push_back(SElem{ InTimeStamp, InValue, 0, ChangingType() });
		}
		else
		{
			TimeType Duration = InTimeStamp - Collection_.back().Stamp_;
			if(FMath::IsNearlyZero(Duration))
			{
				SElem& Elem = Collection_.back();
				Elem.Value_ = InValue;
				if(Collection_.size() > 1)
				{
					Elem.Changed_ = InCalcChangeFunc(InValue, (*(Collection_.rbegin()--)).Value_);
				}
			}
			else
			{
				ChangingType Changed = InCalcChangeFunc(InValue, Collection_.back().Value_);
			
				Collection_.push_back(SElem{ InTimeStamp, InValue, Duration, Changed});

				SumChanges_ += Changed;
				FullDuration_ += Duration;

				while(FullDuration_ > MaxTime_ && Collection_.size() > 1)
				{
					Collection_.pop_front();
					FullDuration_ -= Collection_.front().Duration_;
					SumChanges_ -= Collection_.front().Changed_;
				}
			}
		}
	}

	bool IsValid() const { return Collection_.size() >= 2; }
	
	TimeType GetFullTime() const
	{
		if(!IsValid())
			return 0.;
		return  FullDuration_;
	}
	
	ChangingType GetSumChanging() const
	{
		if(!IsValid())
			return ChangingType();
		return  SumChanges_;
	}

	ChangingType GetSpeed() const
	{
		if(!IsValid())
			return ChangingType();
		return  SumChanges_ / FullDuration_;
	}

	TimeType GetAverageDuration() const
	{
		return !IsValid() ? 0 : FullDuration_ / (Collection_.size() - 1);
	}

	void Clear()
	{
		FullDuration_ = 0;
		SumChanges_ = ChangingType();
		Collection_.clear();
	}

private:
	TimeType MaxTime_ = 3;

	std::list<SElem> Collection_;
	
	TimeType FullDuration_ = 0;
	ChangingType SumChanges_ = ChangingType();
};

inline FTimeChangeCollector<float, float, float> FFloatChangeCollector;

template<typename TimeType>
class FTimeStampCollector
{
public:
	struct SElem
	{
		TimeType Stamp_;
		TimeType Duration_;
	};


	FTimeStampCollector() = default;
	FTimeStampCollector(TimeType InMaxTime): MaxTime_(InMaxTime) {}
	FTimeStampCollector(TimeType InMaxTime, TimeType InDropTimeThreshold): MaxTime_(InMaxTime), DropTimeThreshold_(InDropTimeThreshold) {}

	FTimeStampCollector(const FTimeStampCollector& Other)
	: MaxTime_(Other.MaxTime_), DropTimeThreshold_(Other.DropTimeThreshold_), Collection_(Other.Collection_), FullDuration_(Other.FullDuration_) {}

	FTimeStampCollector(FTimeStampCollector&& Other) noexcept
		: MaxTime_(Other.MaxTime_), DropTimeThreshold_(Other.DropTimeThreshold_), Collection_(std::move(Other.Collection_)), FullDuration_(Other.FullDuration_) {}

	void Add(TimeType InTimeStamp)
	{
		if(Collection_.empty())
		{
			Collection_.push_back(SElem{ InTimeStamp, 0 });
		}
		else
		{
			TimeType Duration = InTimeStamp - Collection_.back().Stamp_;
			if(!FMath::IsNearlyZero(Duration))
			{
				if(Duration > DropTimeThreshold_)
				{
					Clear();
					Collection_.push_back(SElem{ InTimeStamp, 0 });
				}
				else
				{
					Collection_.push_back(SElem{ InTimeStamp, Duration});

					FullDuration_ += Duration;
			
					while(FullDuration_ > MaxTime_ && Collection_.size() > 1)
					{
						Collection_.pop_front();
						FullDuration_ -= Collection_.front().Duration_;
					}
				}
			}
		}
	}

	bool IsValid() const { return Collection_.size() > 1; }
	
	TimeType GetFullTime() const
	{
		if(!IsValid())
			return 0.;
		return  FullDuration_;
	}

	TimeType GetLastDuration() const
	{
		return !IsValid() ? 0 : Collection_.rbegin()->Duration_;
	}

	TimeType GetAverageDuration() const
	{
		return !IsValid() ? 0 : FullDuration_ / (Collection_.size() - 1);
	}

	std::pair<TimeType, TimeType> GetMinMaxDuration() const
	{
		if(Collection_.empty())
			return std::make_pair(0.f, 0.f);

		const auto Elems = std::minmax_element(Collection_.begin(), Collection_.end(), [](const SElem& e1, const SElem& e2) { return e1.Duration_ < e2.Duration_; });
		return std::make_pair(Elems.first->Duration_, Elems.second->Duration_);
	}

	void Clear()
	{
		FullDuration_ = 0;
		Collection_.clear();
	}

	const std::list<SElem>& GetCollection() const { return Collection_; } 

private:
	TimeType MaxTime_ = 3;
	TimeType DropTimeThreshold_ = 0.4f;
	std::list<SElem> Collection_;
	TimeType FullDuration_ = 0;
};

class FDateTimeStampCollector
{
public:
	struct SElem
	{
		FDateTime Stamp_;
		float Duration_;
	};

	FDateTimeStampCollector() = default;
	FDateTimeStampCollector(float InMaxTime): MaxTime_(InMaxTime) {}
	FDateTimeStampCollector(float InMaxTime, float InDropTimeThreshold): MaxTime_(InMaxTime), DropTimeThreshold_(InDropTimeThreshold) {}

	void Add(FDateTime InTimeStamp)
	{
		if(Collection_.empty())
		{
			Collection_.push_back(SElem{ InTimeStamp, 0 });
		}
		else
		{
			
			float Duration = (InTimeStamp - Collection_.back().Stamp_).GetTotalSeconds();
			if(!FMath::IsNearlyZero(Duration))
			{
				if(Duration > DropTimeThreshold_)
				{
					Clear();
					Collection_.push_back(SElem{ InTimeStamp, 0 });
				}
				else
				{
					Collection_.push_back(SElem{ InTimeStamp, Duration});

					FullDuration_ += Duration;
			
					while(FullDuration_ > MaxTime_ && Collection_.size() > 1)
					{
						Collection_.pop_front();
						FullDuration_ -= Collection_.front().Duration_;
					}
				}
			}
		}
	}

	void Clear()
	{
		FullDuration_ = 0;
		Collection_.clear();
	}

	bool IsValid() const { return Collection_.size() > 1; }
	
	float GetFullTime() const
	{
		if(!IsValid())
			return 0.;
		return  FullDuration_;
	}
	
	float GetLastDuration() const
	{
		return !IsValid() ? 0 : Collection_.rbegin()->Duration_;
	}

	FDateTime GetLastTimeStamp() const
	{
		return !IsValid() ? 0 : Collection_.rbegin()->Stamp_;
	}

	float GetAverageDuration() const
	{
		return !IsValid() ? 0 : FullDuration_ / (Collection_.size() - 1);
	}
	
private:
	float MaxTime_ = 3;
	float DropTimeThreshold_ = 0.4f;
	std::list<SElem> Collection_;
	float FullDuration_ = 0;
};