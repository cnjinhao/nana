#include <nana/any.hpp>


namespace nana
{

	//class any
		//struct super_type
			any::super_type::~super_type(){}
			
			any::super_type& any::super_type::operator=(const super_type &rhs)
			{
				return assign(rhs);
			}
		//end struct super_type

		any::any()
			:super_(nullptr)
		{}
		
		any::any(const any& rhs)
			:super_(rhs.super_ ? rhs.super_->clone() : nullptr)
		{}

		any::any(any&& r)
			:super_(r.super_)
		{
			r.super_ = nullptr;
		}
		
		any::~any()
		{
			delete super_;	
		}
		
		any& any::operator=(const any& rhs)
		{
			if(this != &rhs)
			{
				delete super_;
				super_ = (rhs.super_ ? rhs.super_->clone() : nullptr);
			}
			return *this;	
		}

		any& any::operator=(any&& r)
		{
			if(this != &r)
			{
				delete super_;
				super_ = r.super_;
				r.super_ = nullptr;
			}
			return *this;
		}
		
		bool any::same(const any &rhs) const
		{
			if(this != &rhs)
			{
				if(super_ && rhs.super_)
					return super_->same(*rhs.super_);
				else if(super_ || rhs.super_)
					return false;
			}
			return true;
		}
	//end class any
}//end namespace nana

