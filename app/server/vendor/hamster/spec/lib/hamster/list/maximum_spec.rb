require "spec_helper"
require "hamster/list"

describe Hamster::List do
  [:maximum, :max].each do |method|
    describe "##{method}" do
      context "on a really big list" do
        it "doesn't run out of stack" do
          -> { Hamster.interval(0, STACK_OVERFLOW_DEPTH).send(method) }.should_not raise_error
        end
      end

      context "with a block" do
        [
          [[], nil],
          [["A"], "A"],
          [%w[Ichi Ni San], "Ichi"],
        ].each do |values, expected|
          context "on #{values.inspect}" do
            it "returns #{expected.inspect}" do
              Hamster.list(*values).send(method) { |maximum, item| maximum.length <=> item.length }.should == expected
            end
          end
        end
      end

      context "without a block" do
        [
          [[], nil],
          [["A"], "A"],
          [%w[Ichi Ni San], "San"],
        ].each do |values, expected|
          context "on #{values.inspect}" do
            it "returns #{expected.inspect}" do
              Hamster.list(*values).send(method).should == expected
            end
          end
        end
      end
    end
  end
end