<template>
    <div>
        <!--form>
            <div class="form-group row">
                <label for="keywordInput" class="col-sm-2 col-form-label"><b>Search</b></label>
                <div class="col-sm-6">
                    <input class="form-control" id="keywordInput" type="text" ng-model="searchKeyword">
                </div>
            </div>
        </form-->

        <pre>{{ gateways }}</pre>
    </div>

</template>

<script>
    export default {
        name: "AllGatewaysTable",
        props: ['gateways'],
        data() {
            return {}
        },
    }

</script>

