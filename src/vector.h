#pragma once

template <typename T>
class Vector {
	private:
		T *data;

		
	public: 
		void push();
		T pop();

		size_t size();
		
};
